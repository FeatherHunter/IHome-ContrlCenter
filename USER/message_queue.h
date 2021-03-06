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

/*camera's message queue*/
#define CAMERASIZE 6
extern void *camera_q[CAMERASIZE];
extern OS_EVENT * camera_event;

/*send消息队列*/
#define SENDSIZE 20
extern void *tcp_send_q[SENDSIZE];
extern OS_EVENT * tcp_send_event;

/*tcp client handle msg消息队列*/
#define HANDLESIZE 10
extern void *tcp_handle_q[HANDLESIZE];
extern OS_EVENT * tcp_handle_event;

#endif

