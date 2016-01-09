#ifndef _H_MAIN
#define _H_MAIN
#include "message_queue.h"
#include "task_priority.h"
#include "instructions.h"
#include "idebug.h"
#include "key.h"  
#include "lcd.h"
#include "led.h"
#include "malloc.h"

#define TCP_RX_BUFSIZE	    200	  //接收缓冲区长度
#define REMOTE_PORT				  8080	//port: stm32 as client to connect server by ethernet
#define TCP_SERVER_PORT			8080	//port: stm32 as server

extern const char master[ACCOUNT_MAX + 1];
extern const char slave[ACCOUNT_MAX + 1];
extern char password[ACCOUNT_MAX + 1];

#endif
