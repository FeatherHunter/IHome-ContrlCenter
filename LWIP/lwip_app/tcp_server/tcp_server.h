#ifndef __TCP_SERVER_DEMO_H
#define __TCP_SERVER_DEMO_H
#include "sys.h"
#include "includes.h"   
 
#define TCP_SERVER_RX_BUFSIZE	200		//����tcp server���������ݳ���
#define TCP_SERVER_PORT			8080	//����tcp server�Ķ˿�

extern u8 isAccepted;
extern u8 isChecked;
extern struct netconn *serverconn;

INT8U tcp_server_init(void);		//TCP��������ʼ��(����TCP�������߳�)
#endif

