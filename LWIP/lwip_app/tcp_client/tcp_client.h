#ifndef __TCP_CLIENT_DEMO_H
#define __TCP_CLIENT_DEMO_H
#include "sys.h"
#include "includes.h"   

extern u8 isConnected;
extern u8 isAuthed;

#define MASTER "975559549"
#define SLAVE  "975559549h"
 
#define TCP_CLIENT_RX_BUFSIZE	200	//接收缓冲区长度
#define REMOTE_PORT				8080	//定义远端主机的IP地址

extern u8 tcp_client_flag;		//TCP客户端数据发送标志位

INT8U tcp_client_init(void);  //tcp客户端初始化(创建tcp客户端线程)
void tcp_client_connect(void *arg);
void tcp_client_recv(void *arg);
void tcp_client_handle_task(void *arg);
void tcp_client_send(void *arg);

#endif

