/*
 * tcp_echo_server.c
 *
 *  Created on: Jun 1, 2020
 *      Author: andriitkach
 */

#include "lwip/tcp.h"

typedef enum tcp_echo_state_enum {
	ES_NONE = 0,
	ES_ACCEPTED,
	ES_RECEIVED,
	ES_CLOSING
} tcp_echo_state_t;

typedef struct {
	u8_t state;
	u8_t retries;
	struct tcp_pcb * pcb;
	struct pbuf *p;
} tcp_echo_status_t;


err_t tcp_echo_accept_cb(void *arg, struct tcp_pcb * new_pcb, err_t err);
err_t tcp_echo_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
void tcp_echo_err_cb(void *arg, err_t err);
err_t tcp_echo_poll_cb(void *arg, struct tcp_pcb * tpcb);
err_t tcp_echo_sent(void *arg, struct tcp_pcb * tpcb, u16_t len);
void tcp_echo_send(struct tcp_pcb *tpcb, tcp_echo_status_t *es);
void tcp_echo_close(struct tcp_pcb *tpcb, tcp_echo_status_t *es);


void tcp_echo_server_init(void) {
	struct tcp_pcb * tcp_echo_pcb = tcp_new();

	if(tcp_echo_pcb != NULL) {
		err_t tcp_err_val;
		tcp_err_val = tcp_bind(tcp_echo_pcb, IP_ADDR_ANY, 7);
		if(tcp_err_val == ERR_OK) {
			tcp_echo_pcb = tcp_listen(tcp_echo_pcb);
			tcp_accept(tcp_echo_pcb, tcp_echo_accept_cb);
		}
		else  {
			memp_free(MEMP_TCP_PCB, tcp_echo_pcb);
		}

	}
}

err_t tcp_echo_accept_cb(void *arg, struct tcp_pcb * new_pcb, err_t err) {
	err_t tcp_echo_err;
	tcp_echo_status_t *es;
	tcp_setprio(new_pcb, TCP_PRIO_MIN);
	es = (tcp_echo_status_t *)mem_malloc(sizeof(tcp_echo_status_t));
	if(es != NULL) {
		es->pcb = new_pcb;
		es->state = ES_ACCEPTED;
		es->retries = 0;
		es->p = NULL;

		tcp_arg(new_pcb, es);
		tcp_recv(new_pcb, tcp_echo_recv_cb);
		tcp_err(new_pcb, tcp_echo_err_cb);
		tcp_poll(new_pcb, tcp_echo_poll_cb, 0);
		tcp_echo_err = ERR_OK;
	}
	else {
		tcp_echo_err = ERR_MEM;
	}
	return tcp_echo_err;
}

err_t tcp_echo_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
	err_t tcp_echo_err;
	tcp_echo_status_t *es = (tcp_echo_status_t *)arg;
	if(p == NULL) {
		 /* Remote host closed connection */
		es->state = ES_CLOSING;
		if(es->p == NULL) {
			/* We're done sending. Closing the connection */
			tcp_echo_close(tpcb, es);
		}
		else {
			/* Not done yet */
			tcp_sent(tpcb, tcp_echo_sent);
			tcp_echo_send(tpcb, es);
		}
		tcp_echo_err = ERR_OK;
	}
	else if(err != ERR_OK) {
		/* cleanup, for unknown reason */
		if(p != NULL) {
			es->p = NULL;
			pbuf_free(p);
		}
		tcp_echo_err = err;
	}
	else if(es->state == ES_ACCEPTED) {
		/* first data chunk in p->payload */
		es->state = ES_RECEIVED;
		/* store reference to incoming pbuf (chain) */
		es->p = p;
		 /* install send completion notifier */
		tcp_sent(tpcb, tcp_echo_sent);
		tcp_echo_send(tpcb, es);
		tcp_echo_err = ERR_OK;
	}
	else if(es->state == ES_RECEIVED) {
		/* read some more data */
		if(es->p == NULL) {
			es->p = p;
			tcp_sent(tpcb, tcp_echo_sent);
			tcp_echo_send(tpcb, es);
		}
		else {
			/* chain pbufs to the end of what we recv'ed previously  */
			struct pbuf * ptr;
			ptr = es->p;
			pbuf_chain(ptr, p);
		}
		tcp_echo_err = ERR_OK;
	}
	else if(es->state == ES_CLOSING) {
		/* odd case, remote side closing twice, trash data */
		tcp_recved(tpcb, p->tot_len);
		es->p = NULL;
		pbuf_free(p);
		tcp_echo_err = ERR_OK;
	}
	else {
		/* unkown es->state, trash data  */
		tcp_recved(tpcb, p->tot_len);
		es->p = NULL;
		pbuf_free(p);
		tcp_echo_err = ERR_OK;
	}
	return tcp_echo_err;
}

void tcp_echo_err_cb(void *arg, err_t err) {
	tcp_echo_status_t * es;
	es = (tcp_echo_status_t *)arg;
	if(es != NULL) {
		mem_free(es);
	}
}

err_t tcp_echo_poll_cb(void *arg, struct tcp_pcb * tpcb) {
	err_t tcp_echo_err;
	tcp_echo_status_t *es;
	es = (tcp_echo_status_t *)arg;
	if(es != NULL) {
		if(es->p != NULL) {
			/* there is a remaining pbuf (chain)  */
			tcp_sent(tpcb, tcp_echo_sent);
			tcp_echo_send(tpcb, es);
		}
		else {
			/* no remaining pbuf (chain)  */
			if(es->state == ES_CLOSING) {
				tcp_echo_close(tpcb, es);
			}
		}
		tcp_echo_err = ERR_OK;
	}
	else {
		/* nothing to be done */
		tcp_abort(tpcb);
		tcp_echo_err = ERR_ABRT;
	}
	return tcp_echo_err;
}

err_t tcp_echo_sent(void *arg, struct tcp_pcb * tpcb, u16_t len) {
	tcp_echo_status_t *es;

	es = (tcp_echo_status_t *)arg;
	es->retries = 0;
	if(es->p != NULL) {
		tcp_sent(tpcb, tcp_echo_sent);
		tcp_echo_send(tpcb, es);
	}
	else {
		if(es->state == ES_CLOSING) {
			tcp_echo_close(tpcb, es);
		}

	}
	return ERR_OK;
}

void tcp_echo_send(struct tcp_pcb *tpcb, tcp_echo_status_t *es) {
	struct pbuf * ptr;
	err_t tcp_echo_err = ERR_OK;
	while((tcp_echo_err == ERR_OK) && (es->p != NULL) && (es->p->len <= tcp_sndbuf(tpcb))) {
		ptr = es->p;
		tcp_echo_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
		if(tcp_echo_err == ERR_OK) {
			u16_t plen;
			u8_t freed;
			plen = ptr->len;
			es->p = ptr->next;
			if(es->p != NULL) {
				pbuf_ref(es->p);
			}
			do {
				freed = pbuf_free(ptr);
			} while(freed == 0);
			tcp_recved(tpcb, plen);
		}
		else if(tcp_echo_err == ERR_MEM) {
			es->p = ptr;
		}
		else {

		}
	}
}
void tcp_echo_close(struct tcp_pcb *tpcb, tcp_echo_status_t *es) {
	tcp_arg(tpcb, NULL);
	tcp_sent(tpcb, NULL);
	tcp_recv(tpcb, NULL);
	tcp_err(tpcb, NULL);
	tcp_poll(tpcb, NULL, 0);

	if(es != NULL) {
		mem_free(es);
	}
	tcp_close(tpcb);

}


























