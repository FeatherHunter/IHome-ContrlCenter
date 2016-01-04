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
#define HANDLE_MSG_TASK_PRIO       		9
#define HANDLE_MSG_STK_SIZE  		    256
extern OS_STK HANDLE_MSG_TASK_STK[HANDLE_MSG_STK_SIZE];	
void handle_message_task(void *arg);

/*LED任务*/
#define LED_TASK_PRIO		7
#define LED_STK_SIZE		128
extern OS_STK	LED_TASK_STK[LED_STK_SIZE];
void led_task(void *pdata);

/*DHT11任务*/
#define DHT11_TASK_PRIO		8
#define DHT11_STK_SIZE		128
extern OS_STK	DHT11_TASK_STK[DHT11_STK_SIZE];
void dht11_task(void *pdata);

/*start任务*/
#define START_TASK_PRIO		10
#define START_STK_SIZE		128
extern OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 


#endif
