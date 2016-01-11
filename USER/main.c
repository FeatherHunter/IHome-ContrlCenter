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
#include "message_queue.h"
#include "pwm.h"
#include "lsens.h"
#include "main.h"
/*start����*/
OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 

const char master[ACCOUNT_MAX + 1] = "975559549";
const char slave[ACCOUNT_MAX + 1] = "975559549h";
char password[ACCOUNT_MAX + 1] = "545538516";

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
	
	DEBUG_LCD(50,20,"Welcome to IHome", RED);
	
	DEBUG_LCD(20,40,"DHT11 initing....", RED);
	while(DHT11_Init())//��ʪ�ȴ�����
	{
		DEBUG_LCD(20,40,"DHT11 init error!", RED);
		delay_ms(250);
		DEBUG_LCD(20,40, "DHT11 initing....", RED);
		delay_ms(250);
	}
	DEBUG_LCD(20,40, "                 ", RED);
	
	mymem_init(SRAMIN);  	//��ʼ���ڲ��ڴ��
	mymem_init(SRAMEX);  	//��ʼ���ⲿ�ڴ��
	mymem_init(SRAMCCM); 	//��ʼ��CCM�ڴ��

	OSInit(); 					//UCOS��ʼ��
	DEBUG_LCD(20,60,"Lwip Initing.....", RED);
	while(lwip_comm_init()) 	//lwip��ʼ��
	{
		DEBUG_LCD(20,60,"Lwip Init failed!", RED); 	//lwip��ʼ��ʧ��
		delay_ms(500);
		LCD_Fill(20,60,230,150,WHITE);
		delay_ms(500);
	}
	DEBUG_LCD(20,60,"Lwip Init Success!", GREEN); 		//lwip��ʼ���ɹ�
	
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
	
	dht11_event       = OSQCreate(&dht11_q[0]     , DHT11SIZE);
	led_event         = OSQCreate(&led_q[0]       , MSGSIZE);
	tcp_send_event    = OSQCreate(&tcp_send_q[0]  , SENDSIZE);   //����������Ϣ����
  tcp_handle_event  = OSQCreate(&tcp_handle_q[0], HANDLESIZE); //����������Ϣ����
	
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //����DHCP����
#endif
	LED_TASK_STK   = mymalloc(SRAMEX, LED_STK_SIZE*sizeof(OS_STK));
	DHT11_TASK_STK = mymalloc(SRAMEX, DHT11_STK_SIZE*sizeof(OS_STK));
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);//����LED����
 	OSTaskCreate(dht11_task,(void*)0,(OS_STK*)&DHT11_TASK_STK[DHT11_STK_SIZE-1],DHT11_TASK_PRIO);//����DHT11����
	
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
	OS_EXIT_CRITICAL();  		//���ж�
}



