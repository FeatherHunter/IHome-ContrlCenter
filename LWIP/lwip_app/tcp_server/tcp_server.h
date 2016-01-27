#ifndef __TCP_SERVER_DEMO_H
#define __TCP_SERVER_DEMO_H
#include "sys.h"
#include "includes.h"   

extern u8 isAccepted;
extern u8 isChecked;
extern struct netconn *serverconn;
extern struct netconn *video_server;

INT8U tcp_server_init(void);		//TCP服务器初始化(创建TCP服务器线程)
#endif

