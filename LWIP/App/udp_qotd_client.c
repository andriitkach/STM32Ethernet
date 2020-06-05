/*
 * udp_qotd_client.c
 *
 *  Created on: May 29, 2020
 *      Author: andriitkach
 */

#include "lwip/udp.h"
#include "udp_qotd_client.h"
uint8_t req_sent;
ip_addr_t qotd_addr;
u16_t qotd_port;
struct udp_pcb * udp_qotd_pcb;

void udp_qotd_req(void) {
	struct pbuf * p;
	printf("Sendind UDP req\n");
	p = pbuf_alloc(PBUF_TRANSPORT, 10, PBUF_RAM);
	printf(">>>>>>>>>>>>>>>>>>>>>>>>>Sending UDP req\n");
	if(ERR_OK != udp_sendto(udp_qotd_pcb, p, &qotd_addr, qotd_port)) {
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>UDP req FAIL\n");
	} else {
		req_sent++;
		printf(">>>>>>>>>>>>>>>>>>>>>>>>>UDP req SUCCESS\n");
	}
	pbuf_free(p);
}

//void (*udp_recv_fn)(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void udp_qotd_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
	if(p->payload != NULL) {
		printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Received UDP data:\n");
		print_udp_payload(p);
	}
	pbuf_free(p);
}

void udp_qotd_client_init() {
	req_sent = 0;
	udp_qotd_pcb = udp_new();
	if(udp_qotd_pcb == NULL) {
		LWIP_DEBUGF(UDP_DEBUG, ("UDP_QOUTD: canont create a pcb"));
	}
	qotd_port = 17;
	IP4_ADDR(&qotd_addr, 104, 9, 242, 101);
	if(ERR_OK != udp_bind(udp_qotd_pcb, IP_ADDR_ANY, 0)) {
		printf("##########################UDP bind FAIL\n");
	} else {
		printf("##########################UDP bind SUCCESS\n");
	}
	//udp_recv(struct udp_pcb *pcb, udp_recv_fn recv, void *recv_arg)
	udp_recv(udp_qotd_pcb, udp_qotd_recv, NULL);
}

