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
#include "tcp_client.h"
#include "tcp_server.h"

#include "sram.h"   
#include "sdio_sdcard.h"    
#include "malloc.h" 
#include "text.h"	
#include "piclib.h"	
#include "string.h"	
#include "math.h"	
#include "dcmi.h"	
#include "ov2640.h"	
#include "beep.h"	
#include "timer.h"

#include "camera.h"

#define TCP_RX_BUFSIZE	    200	  //接收缓冲区长度
#define REMOTE_PORT				  8080	//port: stm32 as client to connect server by ethernet
#define TCP_SERVER_PORT			8080	//port: stm32 as server
#define VIDEO_SERVER_PORT		9080	//port: video server

extern u8 sd_ok;

extern const char master[ACCOUNT_MAX + 1];
extern const char slave[ACCOUNT_MAX + 1];
extern char password[ACCOUNT_MAX + 1];

#endif
