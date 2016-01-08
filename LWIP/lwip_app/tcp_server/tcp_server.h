#ifndef __TCP_SERVER_DEMO_H
#define __TCP_SERVER_DEMO_H
#include "sys.h"
#include "includes.h"   
 
#define TCP_SERVER_RX_BUFSIZE	200		//定义tcp server最大接收数据长度
#define TCP_SERVER_PORT			8080	//定义tcp server的端口

extern u8 isAccepted;
extern u8 isChecked;
extern struct netconn *serverconn;

INT8U tcp_server_init(void);		//TCP服务器初始化(创建TCP服务器线程)
#endif

