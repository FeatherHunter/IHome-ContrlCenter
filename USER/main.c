#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "idebug.h"
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
#include "dht11.h"
#include "message_queue.h"
#include "pwm.h"
#include "lsens.h"
#include "tcp_client.h"
/*start����*/
OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 

int main(void)
{
	delay_init(168);       	//��ʱ��ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//�жϷ�������
	uart_init(115200);    	//���ڲ���������
	usmart_dev.init(84); 	//��ʼ��USMART
	LED_Init();  			//LED��ʼ��
	KEY_Init();  			//������ʼ��
	LCD_Init();  			//LCD��ʼ��
	FSMC_SRAM_Init();		//SRAM��ʼ��
	
	TIM14_PWM_Init(500-1,84-1);	//PF9 PWM,84M/84=1Mhz����Ƶ��,��װ��ֵ500,����PWMƵ��Ϊ 1M/500=2Khz. 
	Lsens_Init();               //���������ʼ��
	
	while(DHT11_Init())//��ʪ�ȴ�����
	{
		LCD_ShowString(30,30,200,20,16,"DHT11 init error!");
		delay_ms(250);
		LCD_ShowString(30,30,200,20,16,"DHT11 initing....");
		delay_ms(250);
	}
	LCD_ShowString(30,30,200,20,16,  "                 ");
	
	mymem_init(SRAMIN);  	//��ʼ���ڲ��ڴ��
	mymem_init(SRAMEX);  	//��ʼ���ⲿ�ڴ��
	mymem_init(SRAMCCM); 	//��ʼ��CCM�ڴ��
	
	POINT_COLOR = RED; 		//��ɫ����
	LCD_ShowString(30,30,200,20,16,"IHome");

	OSInit(); 					//UCOS��ʼ��
	while(lwip_comm_init()) 	//lwip��ʼ��
	{
		LCD_ShowString(30,110,200,20,16,"Lwip Init failed!"); 	//lwip��ʼ��ʧ��
		delay_ms(500);
		LCD_Fill(30,110,230,150,WHITE);
		delay_ms(500);
	}
	LCD_ShowString(30,110,200,20,16,"Lwip Init Success!"); 		//lwip��ʼ���ɹ�
	
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //����UCOS
}

//start����
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	OSStatInit();  			//��ʼ��ͳ������
	OS_ENTER_CRITICAL();  	//���ж�
	
	dht11_event           = OSQCreate(&dht11_q[0]        , DHT11SIZE);
	led_event             = OSQCreate(&led_q[0]          , MSGSIZE);
	client_send_event     = OSQCreate(&client_send_q[0]  , SENDSIZE);   //����������Ϣ����
  client_handle_event   = OSQCreate(&client_handle_q[0], HANDLESIZE); //����������Ϣ����
	
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //����DHCP����
#endif
	
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);//����LED����
 	OSTaskCreate(dht11_task,(void*)0,(OS_STK*)&DHT11_TASK_STK[DHT11_STK_SIZE-1],DHT11_TASK_PRIO);//����DHT11����
	
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
	OS_EXIT_CRITICAL();  		//���ж�
}

//��ʾ��ַ����Ϣ
void display_task(void *pdata)
{
	while(1)
	{ 
		OSTimeDlyHMSM(0,0,0,500);
	}
}


