#ifndef _H_MESSAGE_QUEUE
#define _H_MESSAGE_QUEUE
#include "includes.h"

/*led消息队列*/
#define MSGSIZE 6
extern void *led_q[MSGSIZE];
extern OS_EVENT * led_event;

/*dht11 消息队列*/
#define DHT11SIZE 6
extern void *dht11_q[DHT11SIZE];
extern OS_EVENT * dht11_event;

/*send消息队列*/
#define SENDSIZE 10
extern void *client_send_q[SENDSIZE];
extern OS_EVENT * client_send_event;

/*tcp client handle msg消息队列*/
#define HANDLESIZE 10
extern void *client_handle_q[HANDLESIZE];
extern OS_EVENT * client_handle_event;

#endif

