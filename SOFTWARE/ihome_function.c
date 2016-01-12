#include "ihome_function.h"
/**
 * @copyright ������(2015~2025) QQ��975559549
 * @Author Feather @version 2.0 @date 2016.1.4
 * @filename ihome_function.c
 * @description IHome�������
 * @FunctionList
 *		1.void led_task(void *pdata); //lamp task
 *		2.void dht11_task(void *pdata); //dht11 task about temp and humi
 */ 
#include "main.h"
#include "lsens.h"
#include "camera.h"
 
/*�������ܼҾ�ģʽ�ı�־
 *START:�Ѿ�����
 *STOP:�ر����ܼҾ�ģʽ*/
int ihome_start_flag = IHome_STOP;
 
/*LED�����ջ*/
OS_STK	* LED_TASK_STK;
/*DHT11�����ջ*/
OS_STK	* DHT11_TASK_STK;
/*CAMERA Task STK*/
OS_STK  * CAMERA_TASK_STK;

/*led��Ϣ����*/
void *led_q[MSGSIZE];
OS_EVENT * led_event;

/*dht11 ��Ϣ����*/
void *dht11_q[DHT11SIZE];
OS_EVENT * dht11_event;
 

/**
 * @Function void camera_task(void *arg)
 * @description ����������Ƭ���浽SD��
 * @Input void * void����
 * @Return NULL
 */
void camera_task(void *arg)
{
	  u8 res;
	  DEBUG("camera task\r\n");
	  My_DCMI_Init();			//DCMI����
	  DCMI_DMA_Init((u32)&LCD->LCD_RAM,0,1,DMA_MemoryDataSize_HalfWord,DMA_MemoryInc_Disable);//DCMI DMA����  
	  //OV2640_ImageWin_Set((1600-lcddev.width)/2,(1200-lcddev.height)/2,lcddev.width,lcddev.height);//1:1��ʵ�ߴ�
		//OV2640_OutSize_Set(lcddev.width,lcddev.height); 
		OV2640_ImageSize_Set(1600, 1200);
		//OV2640_ImageWin_Set(0,0,1600,1200);	//ȫ�ߴ����� 
		//OV2640_OutSize_Set(1600, 1200);
	  //DCMI_Start(); 			//������ʾ����
	  while(1)
	  {
			  //DEBUG("camera task\r\n");
		    //�ȴ����շ���������ͷID
		    //while(camera_server_isready == 0)//�ȵ������󷽽���������Ƶ������
		    //{
			  //    OSTimeDlyHMSM(0,0,1,0);  //��ʱ1s
		    //}
		    /*1:1 real size*/
		    //OV2640_ImageWin_Set((1600-lcddev.width)/2,(1200-lcddev.height)/2,lcddev.width,lcddev.height);//1:1��ʵ�ߴ�
		    //OV2640_OutSize_Set(lcddev.width,lcddev.height); 
		    if(sd_ok)
		    {
					//DCMI_Stop(); //ֹͣ��ʾ��LCD 
			    sw_sdcard_mode();	//�л�ΪSD��ģʽ
			    /*BMP����*/
		      //camera_new_pathname(pname,0);//�õ��ļ���	
		      //res=bmp_encode(pname,0,0,lcddev.width,lcddev.height,0);	
			    /*jpg����*/
			    camera_new_pathname(pname,1);//�õ��ļ���	
			    res=ov2640_jpg_photo(pname);
					//OV2640_ImageWin_Set((1600-lcddev.width)/2,(1200-lcddev.height)/2,lcddev.width,lcddev.height);//1:1��ʵ�ߴ�
					//OV2640_OutSize_Set(lcddev.width,lcddev.height);
			    sw_ov2640_mode();	//�л�ΪOV2640ģʽ
			    if(res)//��������
			    {
					    //Show_Str(30,130,240,16,"д���ļ�����!",16,0);		 
			    }else 
			    {
					    //Show_Str(30,130,240,16,"���ճɹ�!",16,0);
					    //Show_Str(30,150,240,16,"����Ϊ:",16,0);
					    //Show_Str(30+42,150,240,16,pname,16,0);		    
					    //BEEP=1;	//�������̽У���ʾ�������
			    }
					//DCMI_Start(); 			//������ʾ����
		    }
		    else //��ʾSD������
		    {					    
			    //Show_Str(30,130,240,16,"SD������!",16,0);
			    //Show_Str(30,150,240,16,"���չ��ܲ�����!",16,0);			    
		    } 
		    //BEEP=0;			//�رշ����� 
		    OSTimeDlyHMSM(0,0,0,1);  //��ʱ1s
	  }
}
 
/**
 * @Function void led_task(void *pdata);
 * @description ����Ƶ�����
 *						�յ�����Ϣ��ON:���� OFF:�ص� �豸ID
 * @Input void *pdata ����
 * @Return NULL
 */
void led_task(void *pdata)
{
	char * auth_msg = NULL;
	char * msg = NULL;
	char * id_msg = NULL;
	u8 * send_buf;
	u32 lsens;
	INT8U err;
	
	while(1)
	{
		//DEBUG("led task!\r\n");
		LED1 =! LED1;
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
		auth_msg = (char *)OSQPend (led_event,
                400,  //wait 500ms
                &err);
		if(err != OS_ERR_NONE)
		{
			//DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
			continue; //û�н��յ�ָ��
		}
		if(strcmp(auth_msg, "client") == 0)
		{
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
				id_msg = (char *)OSQPend (led_event,
                500,  //wait 2000ms
                &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //û�н��յ�ָ��
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 0);
					/*������ݸ���������*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}		
					/*����״̬��Ϣ*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
																		RES_LAMP,COMMAND_SEPERATOR,
																		LAMP_ON, COMMAND_SEPERATOR, 
																		"0", COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}
			else if(strcmp(msg, "OFF") == 0)
			{
				myfree(SRAMEX, msg);
				id_msg = (char *)OSQPend (led_event,
                 500,  //wait 2000ms
                 &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //û�н��յ�ָ��
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 500);
					/*������ݸ���������*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					/*����״̬��Ϣ*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
 																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		RES_LAMP,COMMAND_SEPERATOR,
 																		LAMP_OFF, COMMAND_SEPERATOR, 
 																		"0", COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}
		
	  }//client
		else if(strcmp(auth_msg, "server") == 0)
		{
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
				id_msg = (char *)OSQPend (led_event,
                500,  //wait 2000ms
                &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //û�н��յ�ָ��
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 0);
					/*������ݸ���������*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}		
					/*����״̬��Ϣ*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%s%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,
																		slave,COMMAND_SEPERATOR,
																		RES_LAMP,COMMAND_SEPERATOR,
																		LAMP_ON, COMMAND_SEPERATOR, 
																		"0", COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}
			else if(strcmp(msg, "OFF") == 0)
			{
				myfree(SRAMEX, msg);
				id_msg = (char *)OSQPend (led_event,
                 500,  //wait 2000ms
                 &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //û�н��յ�ָ��
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 500);
					/*������ݸ���������*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					/*����״̬��Ϣ*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%s%c%c",
 																		COMMAND_RESULT, COMMAND_SEPERATOR,
																		slave         , COMMAND_SEPERATOR,
 																		RES_LAMP      , COMMAND_SEPERATOR,
 																		LAMP_OFF      , COMMAND_SEPERATOR, 
 																		"0"           , COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}//end of msg!=on msg!=off
			
		}//end of server
		
		myfree(SRAMEX, auth_msg);
		myfree(SRAMEX, msg);
		myfree(SRAMEX, id_msg);
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
	char * msg = NULL;
	char * msg1 = NULL;
	char * msg2 = NULL;
	u8 temp, humi;
	u8 * send_buf;
	INT8U err;
	while(1)
	{
		msg = (char *)OSQPend (dht11_event,
                0,  //wait 500ms
                &err);
		if(err != OS_ERR_NONE)
		{
				DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
				continue; //û�н��յ�ָ��
		}
		msg1 = (char *)OSQPend (dht11_event,
                500,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("msg1 OSQPend error :%d \r\n", err);
			myfree(SRAMEX, msg);
			continue; //���մ���
		}
 		msg2 = (char *)OSQPend (dht11_event,
                 500,  //wait 2000ms
                 &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("msg2 OSQPend error %s %d\n", __FILE__, __LINE__);
			myfree(SRAMEX, msg);
			myfree(SRAMEX, msg1);
			continue; //���մ���
		}
		DEBUG("msg1:%s\n", msg1);
		DEBUG("msg2:%s\n", msg2);
		if(strcmp(msg, "client") == 0)
		{
			if(strcmp(msg1, "TEMP") == 0)
			{
				if(strcmp(msg2, "10000") == 0)
				{
					if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
					{
						continue;//��ȡ�¶�ʧ��
					}
					/*�����¶���Ϣ*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c975559549h%c"
																		"%c%c"
																			"%s%c"
																				"%c%c%c",
 																	 COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		 RES_TEMP,COMMAND_SEPERATOR,
 																		  msg2, COMMAND_SEPERATOR,  //�豸��
 																		    temp, COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
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
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c975559549h%c"
																	"%c%c"
																	"%s%c"
																	"%c%c%c",
 																	COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																	RES_HUMI,COMMAND_SEPERATOR,
 			  													msg2, COMMAND_SEPERATOR,  //�豸��
 																	humi, COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}//end of strcmp(msg, "HUMI") == 0
		}
		
		else if(strcmp(msg, "server") == 0)
		{
			if(strcmp(msg1, "TEMP") == 0)
			{
				if(strcmp(msg2, "10000") == 0)
				{
					if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
					{
						continue;//��ȡ�¶�ʧ��
					}
					/*�����¶���Ϣ*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c"
																		"%s%c"
																		"%c%c"
																		"%s%c"
																		"%c%c%c",
 																	 COMMAND_RESULT , COMMAND_SEPERATOR,
																	 slave          , COMMAND_SEPERATOR,
 																	 RES_TEMP       , COMMAND_SEPERATOR,
 																	 msg2           , COMMAND_SEPERATOR,  //�豸��
 																	 temp           , COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
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
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
					sprintf((char *)send_buf, "%c%c%s%c"
																	"%c%c"
																	"%s%c"
																	"%c%c%c",
 																	COMMAND_RESULT,COMMAND_SEPERATOR,slave,COMMAND_SEPERATOR,
 																	RES_HUMI,COMMAND_SEPERATOR,
 			  													msg2, COMMAND_SEPERATOR,  //�豸��
 																	humi, COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}//end of strcmp(msg, "HUMI") == 0
		}//end of server
		
		myfree(SRAMEX, msg);
		myfree(SRAMEX, msg1);
		myfree(SRAMEX, msg2);
		OSTimeDlyHMSM(0,0,0,10);  //��ʱ10ms
 	}
}
