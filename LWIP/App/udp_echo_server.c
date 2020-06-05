/*
 * udp_echo_server.c
 *
 *  Created on: May 28, 2020
 *      Author: andriitkach
 */

#include "lwip/udp.h"

void print_udp_payload(struct pbuf *p) {
	u16_t size = p->len;
	u16_t cnt = 0;
	char * payload_it = (char *)p->payload;
	while(cnt < size) {
		printf("%c", *(payload_it));
		payload_it++;
		cnt++;
	}
	printf("\n");
}

void udp_recv_echo_callback (void *arg, struct udp_pcb *pcb, struct pbuf *p,
		const ip_addr_t *addr, u16_t port) {
	if(p->payload != NULL) {
		printf("Received UDP packet. Payload size: %d\n", p->len);
		print_udp_payload(p);
		printf("Sending it back...\n");
		udp_sendto(pcb, p, addr, port);
		pbuf_free(p);
	}
	else {
		printf("UDP received  packet with NULL payload");
	}
}


void udp_echo_server_init() {
	struct udp_pcb * udp_echo_pcb = udp_new();
	if(udp_echo_pcb == NULL) {
		LWIP_DEBUGF(UDP_DEBUG, ("Cannot create UDP pcb\n"));
		return;
	}
	if(ERR_OK != udp_bind(udp_echo_pcb, IP_ADDR_ANY, 7)) {
		LWIP_DEBUGF(UDP_DEBUG, ("Cannot bind UDP pcb\n"));
		return;
	}
	udp_recv(udp_echo_pcb, udp_recv_echo_callback, NULL);
}
