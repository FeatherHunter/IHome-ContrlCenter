#ifndef _H_MESSAGE_QUEUE
#define _H_MESSAGE_QUEUE
#include "includes.h"

/*led消息队列*/
#define MSGSIZE 2
extern void *led_q[MSGSIZE];
extern OS_EVENT * led_event;

/*dht11 消息队列*/
#define DHT11SIZE 2
extern void *dht11_q[DHT11SIZE];
extern OS_EVENT * dht11_event;

/*send消息队列*/
#define SENDSIZE 10
extern void *send_q[SENDSIZE];
extern OS_EVENT * msg_event;

#endif

