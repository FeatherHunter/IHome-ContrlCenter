#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lwip_comm.h"
#include "LAN8720.h"
#include "usmart.h"
#include "timer.h"
#include "lcd.h"
#include "sram.h"
#include "malloc.h"
#include "lwip_comm.h"
#include "includes.h"
#include "lwipopts.h"
#include "task_priority.h"
#include "tcp_client.h"
#include "dht11.h"
#include "message_queue.h"
/*LED任务*/
OS_STK	LED_TASK_STK[LED_STK_SIZE];
void led_task(void *pdata);
/*led消息队列*/
void *led_q[MSGSIZE];
OS_EVENT * led_event;

/*dht11 任务*/
OS_STK	DHT11_TASK_STK[DHT11_STK_SIZE];
void dht11_task(void *pdata);
/*dht11 消息队列*/
void *dht11_q[DHT11SIZE];
OS_EVENT * dht11_event;

/*start任务*/
OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 

#define SIZE 5
void *ServersMSG[SIZE];

int main(void)
{
	delay_init(168);       	//延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//中断分组配置
	uart_init(115200);    	//串口波特率设置
	usmart_dev.init(84); 	//初始化USMART
	LED_Init();  			//LED初始化
	KEY_Init();  			//按键初始化
	LCD_Init();  			//LCD初始化
	FSMC_SRAM_Init();		//SRAM初始化
	
	while(DHT11_Init())//温湿度传感器
	{
		LCD_ShowString(30,30,200,20,16,"DHT11 init error!");
		delay_ms(250);
		LCD_ShowString(30,30,200,20,16,"DHT11 initing....");
		delay_ms(250);
	}
	LCD_ShowString(30,30,200,20,16,  "                 ");
	
	mymem_init(SRAMIN);  	//初始化内部内存池
	mymem_init(SRAMEX);  	//初始化外部内存池
	mymem_init(SRAMCCM); 	//初始化CCM内存池
	
	POINT_COLOR = RED; 		//红色字体
	LCD_ShowString(30,30,200,20,16,"IHome");

	OSInit(); 					//UCOS初始化
	while(lwip_comm_init()) 	//lwip初始化
	{
		LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip初始化失败
		delay_ms(500);
		LCD_Fill(30,110,230,150,WHITE);
		delay_ms(500);
	}
	LCD_ShowString(30,110,200,20,16,"Lwip Init Success!"); 		//lwip初始化成功
	
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //开启UCOS
}

//start任务
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	OSStatInit();  			//初始化统计任务
	OS_ENTER_CRITICAL();  	//关中断
#if LWIP_DHCP
	//lwip_dhcp_configure();
	lwip_comm_dhcp_creat(); //创建DHCP任务
#endif
	led_event = OSQCreate(&led_q[0], MSGSIZE); //创建led消息队列
	msg_event = OSQCreate(&send_q[0], SENDSIZE); //创建发送消息队列
	dht11_event = OSQCreate(&dht11_q[0], DHT11SIZE); //创建dht11消息队列
	
	OSTaskCreate(handle_message_task,(void*)0,(OS_STK*)&HANDLE_MSG_TASK_STK[HANDLE_MSG_STK_SIZE-1],HANDLE_MSG_TASK_PRIO); //显示任务
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);//创建LED任务
	OSTaskCreate(dht11_task,(void*)0,(OS_STK*)&DHT11_TASK_STK[DHT11_STK_SIZE-1],DHT11_TASK_PRIO);//创建DHT11任务
	OSTaskSuspend(OS_PRIO_SELF); //挂起start_task任务
	OS_EXIT_CRITICAL();  		//开中断
}

//显示地址等信息
void display_task(void *pdata)
{
	while(1)
	{ 
		OSTimeDlyHMSM(0,0,0,500);
	}
}


