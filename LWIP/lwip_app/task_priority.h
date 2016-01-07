#ifndef _H_TASK_PRIORITY
#define	_H_TASK_PRIORITY

#include "sys.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "lwip/mem.h"
#include "lwip_comm.h"

/*lwip DHCP����*/
#define LWIP_DHCP_TASK_PRIO         4
#define LWIP_DHCP_STK_SIZE  		    128
extern OS_STK * LWIP_DHCP_TASK_STK;	
void lwip_dhcp_task(void *pdata); 

/*TCP �ͻ��� ������֤ ����*/
#define TCP_CLIENT_CONNECT_TASK_PRIO       		6
#define TCP_CLIENT_CONNECT_STK_SIZE  		    128
extern OS_STK * TCP_CLIENT_CONNECT_TASK_STK;	
void tcp_client_connect(void *arg);

/*TCP �ͻ��� ���շ�������Ϣ ����*/
#define TCP_CLIENT_RECV_TASK_PRIO       		7
#define TCP_CLIENT_RECV_STK_SIZE  		    128
extern OS_STK TCP_CLIENT_RECV_TASK_STK[TCP_CLIENT_RECV_STK_SIZE];	
void tcp_client_recv(void *arg);

/*����������Ľ��յ�����Ϣ*/
#define CLIENT_HANDLE_TASK_PRIO       		9
#define CLIENT_HANDLE_STK_SIZE  		    128
extern OS_STK * CLIENT_HANDLE_TASK_STK;	
void client_handle_task(void *arg);

/*TCP �ͻ��� ������Ϣ ����*/
#define TCP_CLIENT_SEND_TASK_PRIO       		10
#define TCP_CLIENT_SEND_STK_SIZE  		    128
extern OS_STK TCP_CLIENT_SEND_TASK_STK[TCP_CLIENT_SEND_STK_SIZE];	
void tcp_client_send(void *arg);

/*LED����*/
#define LED_TASK_PRIO		18
#define LED_STK_SIZE		64
extern OS_STK	LED_TASK_STK[LED_STK_SIZE];
void led_task(void *pdata);

/*DHT11����*/
#define DHT11_TASK_PRIO		19
#define DHT11_STK_SIZE		64
extern OS_STK	DHT11_TASK_STK[DHT11_STK_SIZE];
void dht11_task(void *pdata);

/*start����*/
#define START_TASK_PRIO		20
#define START_STK_SIZE		128
extern OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 


#endif
