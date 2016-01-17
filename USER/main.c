#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lwip_comm.h"
#include "LAN8720.h"
#include "usmart.h"
#include "timer.h"
#include "lcd.h"
#include "sram.h"
#include "lwip_comm.h"
#include "includes.h"
#include "lwipopts.h"
#include "dht11.h"
#include "pwm.h"
#include "lsens.h"
#include "main.h"

#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"    
#include "fontupd.h"

//u8 msgbuf[15];			//消息缓存区  	 

u8 sd_ok=1;				//0,sd卡不正常;1,SD卡正常. 

/*start任务*/
OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 

const char master[ACCOUNT_MAX + 1] = "975559549";
const char slave[ACCOUNT_MAX + 1] = "975559549h";
char password[ACCOUNT_MAX + 1] = "545538516";

#if 1

int main(void)
{
	u8 res;							 		   				 
	
	delay_init(168);       	//延时初始化
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//中断分组配置
	uart_init(115200);    	//串口波特率设置
	usmart_dev.init(84); 	//初始化USMART
	LED_Init();  			//LED初始化
	KEY_Init();  			//按键初始化
	LCD_Init();  			//LCD初始化
	BEEP_Init();				//蜂鸣器初始化
	FSMC_SRAM_Init();		//SRAM初始化
	
	TIM3_Int_Init(10000-1,8400-1);//10Khz计数,1秒钟中断一次 
	TIM14_PWM_Init(500-1,84-1);	//PF9 PWM,84M/84=1Mhz计数频率,重装载值500,所以PWM频率为 1M/500=2Khz. 
	Lsens_Init();               //光敏电阻初始化
	W25QXX_Init();				//初始化W25Q128 
	
	mymem_init(SRAMIN);  	//初始化内部内存池
	mymem_init(SRAMEX);  	//初始化外部内存池
	mymem_init(SRAMCCM); 	//初始化CCM内存池
	
	DEBUG_LCD(50,20,"Welcome to IHome", RED);
	exfuns_init();				//为fatfs相关变量申请内存  
  f_mount(fs[0],"0:",1); 		//挂载SD卡 

	POINT_COLOR=RED;
#if 0	
	while(font_init()) 		//检查字库
	{	    
		LCD_ShowString(30,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(30,50,240,66,WHITE);//清除显示	     
		delay_ms(200);				  
	}  
	DEBUG_LCD(20,40,"DHT11 initing....", RED);
	while(DHT11_Init())//温湿度传感器
	{
		DEBUG_LCD(20,40,"DHT11 init error!", RED);
		delay_ms(250);
		DEBUG_LCD(20,40, "DHT11 initing....", RED);
		delay_ms(250);
	}
	DEBUG_LCD(20,40, "                 ", RED);
#endif
	
	res=f_mkdir("0:/PHOTO");		//创建PHOTO文件夹
	if(res!=FR_EXIST&&res!=FR_OK) 	//发生了错误
	{		    
		Show_Str(30,150,240,16,"SD卡错误!",16,0);
		delay_ms(200);				  
		Show_Str(30,170,240,16,"拍照功能将不可用!",16,0);
		sd_ok=0;  	
	} 
	camera_Init();      //Init camera, mode = JPEG	

	OSInit(); 					//UCOS初始化
	
	DEBUG_LCD(20,60,"Lwip Initing.....", RED);
	while(lwip_comm_init()) 	//lwip初始化
	{
		DEBUG_LCD(20,60,"Lwip Init failed!", RED); 	//lwip初始化失败
		delay_ms(500);
		LCD_Fill(20,60,230,150,WHITE);
		delay_ms(500);
	}
	DEBUG_LCD(20,60,"Lwip Init Success!", GREEN); 		//lwip初始化成功
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
	
	dht11_event       = OSQCreate(&dht11_q[0]     , DHT11SIZE);
	led_event         = OSQCreate(&led_q[0]       , MSGSIZE);
	camera_event      = OSQCreate(&camera_q[0]    , CAMERASIZE);
	tcp_send_event    = OSQCreate(&tcp_send_q[0]  , SENDSIZE);   //创建处理消息队列
  tcp_handle_event  = OSQCreate(&tcp_handle_q[0], HANDLESIZE); //创建发送消息队列
	
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //创建DHCP任务
#endif
	LED_TASK_STK    = mymalloc(SRAMEX, LED_STK_SIZE*sizeof(OS_STK));
	DHT11_TASK_STK  = mymalloc(SRAMEX, DHT11_STK_SIZE*sizeof(OS_STK));
	CAMERA_TASK_STK = mymalloc(SRAMEX, CAMERA_STK_SIZE*sizeof(OS_STK));
	
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);//创建LED任务
 	OSTaskCreate(dht11_task,(void*)0,(OS_STK*)&DHT11_TASK_STK[DHT11_STK_SIZE-1],DHT11_TASK_PRIO);//创建DHT11任务
	OSTaskCreate(camera_task,(void*)0,(OS_STK*)&CAMERA_TASK_STK[CAMERA_STK_SIZE-1],CAMERA_TASK_PRIO);//创建CAMERA任务
	
	OSTaskSuspend(OS_PRIO_SELF); //挂起start_task任务
	OS_EXIT_CRITICAL();  		//开中断
}



#endif



