#ifndef __TCP_CLIENT_DEMO_H
#define __TCP_CLIENT_DEMO_H
#include "sys.h"
#include "includes.h"   

extern u8 isConnected;
extern u8 isAuthed;

extern struct netconn *tcp_clientconn;

extern u8 tcp_client_flag;		//TCP客户端数据发送标志位

INT8U tcp_client_init(void);  //tcp客户端初始化(创建tcp客户端线程)
void tcp_client_connect(void *arg);
void tcp_client_recv(void *arg);
void tcp_tcp_handle_task(void *arg);
void tcp_send_task(void *arg);

#endif

