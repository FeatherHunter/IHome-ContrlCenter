#include "idebug.h"
#include "task_priority.h"
#include "ihome_function.h"
#include "led.h"
#include "lsens.h"

/**
 * @copyright ������(2015~2025) QQ��975559549
 * @Author Feather @version 2.0 @date 2016.1.4
 * @filename ihome_function.c
 * @description IHome�������
 * @FunctionList
 *		1.void led_task(void *pdata); //lamp task
 *		2.void dht11_task(void *pdata); //dht11 task about temp and humi
 */ 
 
/*�������ܼҾ�ģʽ�ı�־
 *START:�Ѿ�����
 *STOP:�ر����ܼҾ�ģʽ*/
int ihome_start_flag = IHome_STOP;
 
 
/**
 * @Function void led_task(void *pdata);
 * @description ����Ƶ�����
 *						�յ�����Ϣ��ON:���� OFF:�ص� �豸ID
 * @Input void *pdata ����
 * @Return NULL
 */
void led_task(void *pdata)
{
	char * msg = NULL;
	u8 * send_buf;
	u32 lsens;
	INT8U err;
	
	while(1)
	{
		DEBUG("led task!\n");
		if(ihome_start_flag == IHome_START)//������IHomeģʽ
		{
			lsens = (Lsens_Get_Val()+10)*5;
			if(lsens >= 500)
			{
				TIM_SetCompare1(TIM14, 500);
			}
			else
			{
				TIM_SetCompare1(TIM14, lsens);
			}
		}
		msg = (char *)OSQPend (led_event,
                500,  //wait 500ms
                &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
			continue; //û�н��յ�ָ��
		}
		if(strcmp(msg, "ON") == 0)
		{
 			myfree(SRAMEX, msg);
			msg = (char *)OSQPend (led_event,
                2000,  //wait 2000ms
                &err);
 			if(strcmp(msg, "0") == 0)
 			{
 				TIM_SetCompare1(TIM14, 0);
				/*����״̬��Ϣ*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
				sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
																		RES_LAMP,COMMAND_SEPERATOR,
																		LAMP_ON, COMMAND_SEPERATOR, 
																		"0", COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
 				tcp_client_flag|=1<<7;//���Ҫ��������
 			}
		}
 		else if(strcmp(msg, "OFF") == 0)
 		{
 			myfree(SRAMEX, msg);
 			msg = (char *)OSQPend (led_event,
                 2000,  //wait 2000ms
                 &err);
			if(strcmp(msg, "0") == 0)
 			{
 				TIM_SetCompare1(TIM14, 500);
				/*����״̬��Ϣ*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
				sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
 																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		RES_LAMP,COMMAND_SEPERATOR,
 																		LAMP_OFF, COMMAND_SEPERATOR, 
 																		"0", COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
 				tcp_client_flag|=1<<7;//���Ҫ��������
 			}
 		}
		myfree(SRAMEX, msg);
		OSTimeDlyHMSM(0,0,0,10);  //��ʱ10ms
 	}
}

/**
 * @Function void dht11_task(void *pdata)
 * @description �����¶Ⱥ�ʪ�ȵ�����
 *						�յ�����Ϣ��TEMP:�¶� HUMI:ʪ�� �豸ID
 * @Input void *pdata ����
 * @Return NULL
 */
void dht11_task(void *pdata)
{
	char * msg1 = NULL;
	char * msg2 = NULL;
	u8 temp, humi;
	u8 * send_buf;
	INT8U err;
	
	while(1)
	{
		DEBUG("dht11 task!\n");
		msg1 = (char *)OSQPend (dht11_event,
                0,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("msg1 OSQPend error %s %d\n", __FILE__, __LINE__);
			continue; //���մ���
		}
 		msg2 = (char *)OSQPend (dht11_event,
                 2000,  //wait 2000ms
                 &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("msg2 OSQPend error %s %d\n", __FILE__, __LINE__);
			continue; //���մ���
		}
		DEBUG("msg1:%s\n", msg1);
		DEBUG("msg2:%s\n", msg2);
		if(strcmp(msg1, "TEMP") == 0)
		{
			if(strcmp(msg2, "10000") == 0)
 			{
				if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
				{
					continue;//��ȡ�¶�ʧ��
				}
				/*�����¶���Ϣ*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
				sprintf((char *)send_buf, "%c%c975559549h%c"
																		"%c%c"
																			"%s%c"
																				"%c%c%c",
 																	 COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		 RES_TEMP,COMMAND_SEPERATOR,
 																		  msg2, COMMAND_SEPERATOR,  //�豸��
 																		    temp, COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
 				tcp_client_flag|=1<<7;//���Ҫ��������
 			}

		}//end of (strcmp(msg, "TEMP") == 0)
		else if(strcmp(msg1, "HUMI") == 0)
		{
			if(strcmp(msg2, "10000") == 0)
 			{
				if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
				{
					continue;//��ȡ�¶�ʧ��
				}
				/*����ʪ����Ϣ*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
				sprintf((char *)send_buf, "%c%c975559549h%c"
																	"%c%c"
																	"%s%c"
																	"%c%c%c",
 																	COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																	RES_HUMI,COMMAND_SEPERATOR,
 			  													msg2, COMMAND_SEPERATOR,  //�豸��
 																	humi, COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
				
 				tcp_client_flag|=1<<7;//���Ҫ��������
 			}
		}//end of strcmp(msg, "HUMI") == 0
		
		myfree(SRAMEX, msg1);
		myfree(SRAMEX, msg2);
		OSTimeDlyHMSM(0,0,0,10);  //��ʱ10ms
 	}
}
