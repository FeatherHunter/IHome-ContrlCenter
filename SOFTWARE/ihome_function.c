/**
 * @copyright ������(2015~2025) QQ��975559549
 * @Author Feather @version 2.0 @date 2016.1.4
 * @filename ihome_function.c
 * @description IHome�������
 * @FunctionList
 *		1.void led_task(void *pdata); //lamp task
 *		2.void dht11_task(void *pdata); //dht11 task about temp and humi
 */ 
#include "ihome_function.h"
#include "main.h"
#include "lsens.h"
#include "camera.h"
#include "delay.h"

#include "lwip/opt.h"
#include "lwip_comm.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"
#include "includes.h"
 
/*�������ܼҾ�ģʽ�ı�־
 *START:�Ѿ�����
 *STOP:�ر����ܼҾ�ģʽ*/

#define CAMERA_ID "20000"
 
int ihome_start_flag = IHome_STOP;

int camera_start = VIDEO_STOP;
 
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

/*camera's message queue*/
void *camera_q[CAMERASIZE];
OS_EVENT * camera_event;
 

/**
 * @Function void camera_task(void *arg)
 * @description ����������Ƭ���浽SD��
 * @Input void * void����
 * @Return NULL
 */
void camera_task(void *arg)
{
	  u8 res = 0;
		u8 isServer = 1;
	  INT8U err;
		u8 * send_buf;
	  u8 * startflag_buf;
		u8 * endflag_buf;
		u8* pbuf;
		u16 i;
		unsigned int len;
	  unsigned int count;
		unsigned int jpegtimes;
		unsigned int jpegremainder;
		//char length[10];
		char * msg_auth;
	  char * msg_camera;
	  DEBUG("camera task\r\n");
	  My_DCMI_Init();			//DCMI����
	  DCMI_DMA_Init((u32)&LCD->LCD_RAM,0,1,DMA_MemoryDataSize_HalfWord,DMA_MemoryInc_Disable);//DCMI DMA����  
	  //OV2640_ImageWin_Set((1600-lcddev.width)/2,(1200-lcddev.height)/2,lcddev.width,lcddev.height);//1:1��ʵ�ߴ�
		//OV2640_OutSize_Set(lcddev.width,lcddev.height); 
		ov2640_mode=1;
		sw_ov2640_mode();														//�л�ΪOV2640ģʽ
		dcmi_rx_callback=jpeg_dcmi_rx_callback;			//�ص�����
		DCMI_DMA_Init((u32)jpeg_buf0,(u32)jpeg_buf1,jpeg_dma_bufsize,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA����(˫����ģʽ)
		OV2640_JPEG_Mode();												//�л�ΪJPEGģʽ 
		OV2640_ImageWin_Set(0,0, 1600, 1200);			 
		OV2640_OutSize_Set(1600, 1200);						//���ճߴ�Ϊ1600*1200
	  while(1)
	  {
				while(camera_start == VIDEO_STOP)
				{
					OSTimeDlyHMSM(0,0,2,0);  //��ʱ2s
				}
				/*get video's id*/
			  msg_auth = (char *)OSQPend (camera_event,
                0,  //wait 500ms
                &err);
				if(err != OS_ERR_NONE)
				{
						DEBUG("Camera TASK OSQPend error %s %d\n", __FILE__, __LINE__);
						continue; //û�н��յ�ID
				}
				if(strcmp(msg_auth, "server") == 0)
				{
					isServer = 1;
				}
				else if(strcmp(msg_auth, "client") == 0)
				{
					isServer = 0;
				}
				else continue;//not serverconn or client
				msg_camera = (char *)OSQPend (camera_event,
                500,  //wait 500ms
                &err);
				if(err != OS_ERR_NONE)
				{
					DEBUG("Camera TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //û�н��յ�ָ��
				}

//				
//				if(strcmp(msg_camera, CAMERA_ID) != 0)//make sure id's right
//				{
//					/*VIDEO_ERROR*/
//					send_buf = mymalloc(SRAMEX, 41); //32(account))+8+1 ״ָ̬���������
//					sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%c",
//																		COMMAND_RESULT,COMMAND_SEPERATOR,
//																		slave,COMMAND_SEPERATOR,
//																		RES_VIDEO,COMMAND_SEPERATOR,
//																		VIDEO_ERROR, COMMAND_SEPERATOR,COMMAND_END);
//					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
//					{
//						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
//					}
//					continue;
//				}
				/*VIDEO_send start*/
				startflag_buf = mymalloc(SRAMEX, 72); //64+8 ״ָ̬���������
				sprintf((char *)startflag_buf, "%c%c""%s%c""%s%c""%c%c%c",
																		COMMAND_VIDEO,COMMAND_SEPERATOR,
																		slave,COMMAND_SEPERATOR,
																		CAMERA_ID,COMMAND_SEPERATOR,
																		VIDEO_START, COMMAND_SEPERATOR,COMMAND_END);
				/*VIDEO_send stop*/
				endflag_buf = mymalloc(SRAMEX, 72); //64+8 ״ָ̬���������
				sprintf((char *)endflag_buf, "%c%c""%s%c""%s%c""%c%c%c",
																		COMMAND_VIDEO,COMMAND_SEPERATOR,
																		slave,COMMAND_SEPERATOR,
																		CAMERA_ID,COMMAND_SEPERATOR,
																		VIDEO_STOP, COMMAND_SEPERATOR,COMMAND_END);
				while(camera_start == VIDEO_START)//start video
				{
					
					if(isServer == 1)
					{
						//send start
						err = netconn_write(serverconn ,startflag_buf,strlen((char*)startflag_buf),NETCONN_COPY); //���͸��û���ʼ�������Ϣ
						if(err != ERR_OK)
						{
								DEBUG("����ʧ��\r\n");
								isAccepted = 0;
								camera_start = VIDEO_STOP;
								continue;
						}
						DEBUG("video send START to user\r\n");
					}
					else
					{
						err = netconn_write(tcp_clientconn ,startflag_buf,strlen((char*)startflag_buf),NETCONN_COPY); //���͸��û���ʼ�������Ϣ
						if(err != ERR_OK)
						{
							DEBUG("����ʧ��\r\n");
							isConnected = 0;
							camera_start = VIDEO_STOP;
							continue;
						}
						DEBUG("video send START to server\r\n");
					}
					
					DCMI_Start(); 										//�������� 
					while(jpeg_data_ok!=1);						//�ȴ���һ֡ͼƬ�ɼ���
					jpeg_data_ok=2;										//���Ա�֡ͼƬ,������һ֡�ɼ�
					while(jpeg_data_ok!=1);						//�ȴ��ڶ�֡ͼƬ�ɼ���
					jpeg_data_ok=2;										//���Ա�֡ͼƬ,������һ֡�ɼ�
					while(jpeg_data_ok!=1);						//�ȴ�����֡ͼƬ�ɼ���,����֡,�ű��浽SD��ȥ.
					DCMI_Stop(); 											//ֹͣDMA����
					
					/*get a jpeg photo*/
					pbuf=(u8*)jpeg_data_buf;
					for(i=0;i<jpeg_data_len*4;i++)//����0XFF,0XD8
					{
						if((pbuf[i]==0XFF)&&(pbuf[i+1]==0XD8))break;
					}
					if(i==jpeg_data_len*4)res=0XFD;//û�ҵ�0XFF,0XD8
					else//�ҵ���
					{
						pbuf+=i;//ƫ�Ƶ�0XFF,0XD8��
						jpegtimes     = (jpeg_data_len*4-i) / 1000; //�������ļ��ֽ��С�ļ�
						jpegremainder = (jpeg_data_len*4-i) % 1000; //��������ҲҪ���ͳ�ȥ
						
						/*buffer's start*/
						send_buf = mymalloc(SRAMEX, 1100); //1100
						sprintf((char *)send_buf, "%c%c%s%c""%s%c1000%c",
																		COMMAND_VIDEO,COMMAND_SEPERATOR,slave, COMMAND_SEPERATOR,
																		CAMERA_ID,COMMAND_SEPERATOR,COMMAND_SEPERATOR);
						len = strlen(send_buf);
						
						if(isServer == 1)//as server sendto user
						{
							for(count = 0; count < (jpegtimes-30); count++)
							{
								DEBUG("camera task send to user��%d\r\n", count);
								memcpy(send_buf + len, pbuf, 1000); //pbuf��ʼ��4000�ֽڴ���send_buf ͷ��Ϣ֮��
								*(send_buf + len + 1000) = COMMAND_END;
								*(send_buf + len + 1001) = '\0';
							 /*send_buf not send_buf + len*/
								err = netconn_write(serverconn ,send_buf, len + 1001,NETCONN_COPY); //packet 4000byte into a packet
								pbuf += 1000;
								if(err != ERR_OK)
								{
									DEBUG("����ʧ��\r\n");
									isAccepted = 0;
									camera_start = VIDEO_STOP;
									break;
								}
								OSTimeDlyHMSM(0,0,0,10);  //��ʱ10ms
							}
							err = netconn_write(serverconn ,endflag_buf,strlen((char*)endflag_buf),NETCONN_COPY); //sendto user video is end
							if(err != ERR_OK)
							{
								DEBUG("����ʧ��\r\n");
								isAccepted = 0;
								camera_start = VIDEO_STOP;
								continue;
							}
							DEBUG("video send STOP to user\r\n");
						}
						else//as client sendto to server
						{
							for(count = 0; count < (jpegtimes-30); count++)
							{
								DEBUG("camera task send to user��%d\r\n", count);
								memcpy(send_buf + len, pbuf, 1000); //pbuf��ʼ��4000�ֽڴ���send_buf ͷ��Ϣ֮��
								*(send_buf + len + 1000) = COMMAND_END;
								*(send_buf + len + 1001) = '\0';
							 /*send_buf not send_buf + len*/
								err = netconn_write(tcp_clientconn ,send_buf, len + 1001,NETCONN_COPY); //packet 4000byte into a packet
								pbuf += 1000;
								if(err != ERR_OK)
								{
									DEBUG("����ʧ��\r\n");
									isAccepted = 0;
									camera_start = VIDEO_STOP;
									break;
								}
								OSTimeDlyHMSM(0,0,0,10);  //��ʱ10ms
							}
							err = netconn_write(tcp_clientconn ,endflag_buf,strlen((char*)endflag_buf),NETCONN_COPY); //sendto user video is end
							if(err != ERR_OK)
							{
								DEBUG("����ʧ��\r\n");
								isAccepted = 0;
								camera_start = VIDEO_STOP;
								continue;
							}
							DEBUG("video send STOP to user\r\n");
						}//end of if(isServer)
						myfree(SRAMEX, send_buf);
					}
					
					jpeg_data_len=0;
					sw_ov2640_mode();		//�л�ΪOV2640ģʽ

					if(res)
					{
						DEBUG("ov2640_jpg_send, error:%x \r\n", res);
					}
					else
					{
						DEBUG("ov2640_jpg_send, success:%x \r\n", res);
					}
					OSTimeDlyHMSM(0,0,0,10);  //��ʱ50ms
				}//end of while(camera_start == VIDEO_START)

				myfree(SRAMEX, startflag_buf); //free start & end flag buf
				myfree(SRAMEX, endflag_buf);
				
				OSTimeDlyHMSM(0,0,0,1);  //��ʱ1ms
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
