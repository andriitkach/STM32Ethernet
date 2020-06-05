/*
 * udp_echo_server.h
 *
 *  Created on: May 29, 2020
 *      Author: andriitkach
 */

#ifndef _UDP_ECHO_SERVER_H_
#define _UDP_ECHO_SERVER_H_

void udp_echo_server_init(void);
void print_udp_payload(struct pbuf *);

#endif /* _UDP_ECHO_SERVER_H_ */
