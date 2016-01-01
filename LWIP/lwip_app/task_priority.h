#ifndef _H_TASK_PRIORITY
#define	_H_TASK_PRIORITY
#include "sys.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "lwip/mem.h"
#include "lwip_comm.h"

/*lwip DHCP任务*/
#define LWIP_DHCP_TASK_PRIO         4
#define LWIP_DHCP_STK_SIZE  		    128
extern OS_STK * LWIP_DHCP_TASK_STK;	
void lwip_dhcp_task(void *pdata); 

/*连接认证任务*/
#define TCP_CONNECT_TASK_PRIO       		6
#define TCP_CONNECT_STK_SIZE  		    128
extern OS_STK TCP_CONNECT_TASK_STK[TCP_CONNECT_STK_SIZE];	
void tcp_connect_task(void *arg);

/*接受信息并且处理*/
#define HANDLE_MSG_TASK_PRIO       		7
#define HANDLE_MSG_STK_SIZE  		    128
extern OS_STK HANDLE_MSG_TASK_STK[HANDLE_MSG_STK_SIZE];	
void handle_message_task(void *arg);

#endif
