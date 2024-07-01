
#ifndef __MY_UDP_H__
#define __MY_UDP_H__

#ifdef __cplusplus
extern "C" {
#endif

void udp_create_server_task(void);
void udp_send_msg(char *msg, int len);

#ifdef __cplusplus
}
#endif

#endif
