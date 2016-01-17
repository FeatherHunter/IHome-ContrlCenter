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

//u8 msgbuf[15];			//��Ϣ������  	 

u8 sd_ok=1;				//0,sd��������;1,SD������. 

/*start����*/
OS_STK START_TASK_STK[START_STK_SIZE];
void start_task(void *pdata); 

const char master[ACCOUNT_MAX + 1] = "975559549";
const char slave[ACCOUNT_MAX + 1] = "975559549h";
char password[ACCOUNT_MAX + 1] = "545538516";

#if 1

int main(void)
{
	u8 res;							 		   				 
	
	delay_init(168);       	//��ʱ��ʼ��
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//�жϷ�������
	uart_init(115200);    	//���ڲ���������
	usmart_dev.init(84); 	//��ʼ��USMART
	LED_Init();  			//LED��ʼ��
	KEY_Init();  			//������ʼ��
	LCD_Init();  			//LCD��ʼ��
	BEEP_Init();				//��������ʼ��
	FSMC_SRAM_Init();		//SRAM��ʼ��
	
	TIM3_Int_Init(10000-1,8400-1);//10Khz����,1�����ж�һ�� 
	TIM14_PWM_Init(500-1,84-1);	//PF9 PWM,84M/84=1Mhz����Ƶ��,��װ��ֵ500,����PWMƵ��Ϊ 1M/500=2Khz. 
	Lsens_Init();               //���������ʼ��
	W25QXX_Init();				//��ʼ��W25Q128 
	
	mymem_init(SRAMIN);  	//��ʼ���ڲ��ڴ��
	mymem_init(SRAMEX);  	//��ʼ���ⲿ�ڴ��
	mymem_init(SRAMCCM); 	//��ʼ��CCM�ڴ��
	
	DEBUG_LCD(50,20,"Welcome to IHome", RED);
	exfuns_init();				//Ϊfatfs��ر��������ڴ�  
  f_mount(fs[0],"0:",1); 		//����SD�� 

	POINT_COLOR=RED;
#if 0	
	while(font_init()) 		//����ֿ�
	{	    
		LCD_ShowString(30,50,200,16,16,"Font Error!");
		delay_ms(200);				  
		LCD_Fill(30,50,240,66,WHITE);//�����ʾ	     
		delay_ms(200);				  
	}  
	DEBUG_LCD(20,40,"DHT11 initing....", RED);
	while(DHT11_Init())//��ʪ�ȴ�����
	{
		DEBUG_LCD(20,40,"DHT11 init error!", RED);
		delay_ms(250);
		DEBUG_LCD(20,40, "DHT11 initing....", RED);
		delay_ms(250);
	}
	DEBUG_LCD(20,40, "                 ", RED);
#endif
	
	res=f_mkdir("0:/PHOTO");		//����PHOTO�ļ���
	if(res!=FR_EXIST&&res!=FR_OK) 	//�����˴���
	{		    
		Show_Str(30,150,240,16,"SD������!",16,0);
		delay_ms(200);				  
		Show_Str(30,170,240,16,"���չ��ܽ�������!",16,0);
		sd_ok=0;  	
	} 
	camera_Init();      //Init camera, mode = JPEG	

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
	camera_event      = OSQCreate(&camera_q[0]    , CAMERASIZE);
	tcp_send_event    = OSQCreate(&tcp_send_q[0]  , SENDSIZE);   //����������Ϣ����
  tcp_handle_event  = OSQCreate(&tcp_handle_q[0], HANDLESIZE); //����������Ϣ����
	
#if LWIP_DHCP
	lwip_comm_dhcp_creat(); //����DHCP����
#endif
	LED_TASK_STK    = mymalloc(SRAMEX, LED_STK_SIZE*sizeof(OS_STK));
	DHT11_TASK_STK  = mymalloc(SRAMEX, DHT11_STK_SIZE*sizeof(OS_STK));
	CAMERA_TASK_STK = mymalloc(SRAMEX, CAMERA_STK_SIZE*sizeof(OS_STK));
	
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);//����LED����
 	OSTaskCreate(dht11_task,(void*)0,(OS_STK*)&DHT11_TASK_STK[DHT11_STK_SIZE-1],DHT11_TASK_PRIO);//����DHT11����
	OSTaskCreate(camera_task,(void*)0,(OS_STK*)&CAMERA_TASK_STK[CAMERA_STK_SIZE-1],CAMERA_TASK_PRIO);//����CAMERA����
	
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
	OS_EXIT_CRITICAL();  		//���ж�
}



#endif



