#ifndef _H_TASK_PRIORITY
#define	_H_TASK_PRIORITY

/**
 * @copyright 王辰浩(2015~2025) QQ：975559549
 * @Author Feather @version 2.0 @date 2016.1.9
 * @filename task_priority.h
 * @description 宏定义了所有任务的优先级,堆栈大小,便于修改
 */ 

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


/*TCP client*/
#define TCP_CLIENT_CONNECT_TASK_PRIO       		6
#define TCP_CLIENT_CONNECT_STK_SIZE  		    128
extern OS_STK * TCP_CLIENT_CONNECT_TASK_STK;	
void tcp_client_connect(void *arg);

/*tcp server*/
#define TCPSERVER_PRIO		7
#define TCPSERVER_STK_SIZE	300
extern OS_STK * TCPSERVER_TASK_STK;
void tcp_server_task(void *arg);

/*TCP client receive message task*/
#define TCP_CLIENT_RECV_TASK_PRIO       		8
#define TCP_CLIENT_RECV_STK_SIZE  		    128
extern OS_STK * TCP_CLIENT_RECV_TASK_STK;	
void tcp_client_recv(void *arg);

/*TCP server receive message task*/
#define TCPSERVER_RECV_TASK_PRIO       		9
#define TCPSERVER_RECV_STK_SIZE  		    128
extern OS_STK * TCPSERVER_RECV_TASK_STK;	
void tcp_server_recv(void *arg);

/*处理控制中心接收到的消息*/
#define CLIENT_HANDLE_TASK_PRIO       		10
#define CLIENT_HANDLE_STK_SIZE  		    128
extern OS_STK * CLIENT_HANDLE_TASK_STK;	
void tcp_handle_task(void *arg);

/*tcp send msg task*/
#define TCP_SEND_TASK_PRIO       	11
#define TCP_SEND_STK_SIZE  		    128
extern OS_STK * TCP_SEND_TASK_STK;	
void tcp_send_task(void *arg);

/*video tcp server*/
#define VIDEOSERVER_PRIO		7
#define VIDEOSERVER_STK_SIZE	300
extern OS_STK * VIDEOSERVER_TASK_STK;
void video_server_task(void *arg);

/*LED任务*/
#define LED_TASK_PRIO		17
#define LED_STK_SIZE		128
extern OS_STK	* LED_TASK_STK;
void led_task(void *pdata);

/*DHT11任务*/
#define DHT11_TASK_PRIO		18
#define DHT11_STK_SIZE		128
extern OS_STK	* DHT11_TASK_STK;
void dht11_task(void *pdata);

/*camera task*/
#define CAMERA_TASK_PRIO		19
#define CAMERA_STK_SIZE		256
extern OS_STK	* CAMERA_TASK_STK;
void camera_task(void *pdata);

/*start任务*/
#define START_TASK_PRIO		20
#define START_STK_SIZE		128
extern OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 


#endif
