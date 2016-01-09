#include "main.h"
#include "tcp_client.h"
#include "tcp_server.h"
#include "message_queue.h"
#include "task_priority.h"
#include "ihome_function.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"
#include "includes.h"

struct netconn *tcp_clientconn;					//TCP CLIENT�������ӽṹ��
u8 tcp_client_flag;		//TCP�ͻ������ݷ��ͱ�־λ

u8 isConnected = 0;
u8 isAuthed    = 0;

/*TCP �ͻ��˶�ջ*/
OS_STK * TCP_CLIENT_CONNECT_TASK_STK;	//����
OS_STK * TCP_CLIENT_RECV_TASK_STK;	  //����
OS_STK * CLIENT_HANDLE_TASK_STK;	    //����
/*TCP send stack*/
OS_STK * TCP_SEND_TASK_STK;	          //����

/*handle��Ϣ����*/
void *tcp_handle_q[HANDLESIZE];
OS_EVENT * tcp_handle_event;
/*send��Ϣ����*/
void *tcp_send_q[SENDSIZE];
OS_EVENT * tcp_send_event;

/**
 * @Function: INT8U tcp_client_init(void)
 * @Author:   feather
 * @Description: create tasks for stm32 as client
 * @Return 0:     TCP client create success
 *				 other: TCP client create failed
 */
INT8U tcp_client_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	TCP_SEND_TASK_STK           = mymalloc(SRAMEX, TCP_SEND_STK_SIZE*sizeof(OS_STK));
	TCP_CLIENT_CONNECT_TASK_STK = mymalloc(SRAMEX, TCP_CLIENT_CONNECT_STK_SIZE*sizeof(OS_STK));
	CLIENT_HANDLE_TASK_STK      = mymalloc(SRAMEX, CLIENT_HANDLE_STK_SIZE*sizeof(OS_STK));
	TCP_CLIENT_RECV_TASK_STK    = mymalloc(SRAMEX, TCP_CLIENT_RECV_STK_SIZE*sizeof(OS_STK));
	
	OS_ENTER_CRITICAL();	//���ж�
	
	res = OSTaskCreate(tcp_client_connect ,(void*)0, (OS_STK*)&TCP_CLIENT_CONNECT_TASK_STK[TCP_CLIENT_CONNECT_STK_SIZE-1], TCP_CLIENT_CONNECT_TASK_PRIO); //����TCP�ͻ�����������
	res += OSTaskCreate(tcp_client_recv   ,(void*)0, (OS_STK*)&TCP_CLIENT_RECV_TASK_STK[TCP_CLIENT_RECV_STK_SIZE-1]      , TCP_CLIENT_RECV_TASK_PRIO);    //����TCP�ͻ��˽�������
	res += OSTaskCreate(tcp_handle_task,(void*)0, (OS_STK*)&CLIENT_HANDLE_TASK_STK[CLIENT_HANDLE_STK_SIZE-1]          , CLIENT_HANDLE_TASK_PRIO);      //����TCP�ͻ��˴�������
	res += OSTaskCreate(tcp_send_task   ,(void*)0, (OS_STK*)&TCP_SEND_TASK_STK[TCP_SEND_STK_SIZE-1]      , TCP_SEND_TASK_PRIO);    //����TCP�ͻ��˷�������
	
	OS_EXIT_CRITICAL();		//���ж�
	
	return res;
}

//tcp�ͻ���������
void tcp_client_connect(void *arg)
{
	u8 * send_buf; //������Ϣ�Ļ���
	err_t err;
	static ip_addr_t server_ipaddr,loca_ipaddr;
	static u16_t 		 server_port,loca_port;

	LWIP_UNUSED_ARG(arg);
	server_port = REMOTE_PORT;
	IP4_ADDR(&server_ipaddr, lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
	DEBUG("tcp client connect task\r\n");
	while (1) 
	{
		while(isConnected && isAuthed)
		{
			OSTimeDlyHMSM(0,0,2,0);//����������˯��2s
		}
		/*--------��������---------*/
		if(isConnected == 0)
		{
			tcp_clientconn = netconn_new(NETCONN_TCP);  //����һ��TCP����
			err = netconn_connect(tcp_clientconn,&server_ipaddr,server_port);//���ӷ�����
			if(err != ERR_OK)  netconn_delete(tcp_clientconn); //����ֵ������ERR_OK,ɾ��tcp_clientconn����
			else
			{
				netconn_getaddr(tcp_clientconn,&loca_ipaddr,&loca_port,1); //��ȡ����IP����IP��ַ�Ͷ˿ں�
			  printf("�����Ϸ�����%d.%d.%d.%d,�����˿ں�Ϊ:%d\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3],loca_port);
				isConnected = 1;
			  isAuthed = 0;
			}
			POINT_COLOR = BLUE;
			LCD_ShowString(20,100,200,20,16,"connecting...");//��ʾ���յ�������	
			DEBUG("is connecting!\n");
		}
		/*--------��֤��Ϣ---------*/
		if((isConnected != 0)&&(isAuthed == 0))
		{
			/*�������ָ��*/
			send_buf = mymalloc(SRAMEX, 7);
			sprintf((char *)send_buf, "client");
			if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
			}		
			
			send_buf = mymalloc(SRAMEX, 73); //32(account)+32(password)+7+1 ��֤��Ϣ�������
			sprintf((char *)send_buf, "%c%c975559549h%c%c%c975559549%c%c",COMMAND_MANAGE,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR,
																			MAN_LOGIN,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR, COMMAND_END);
			if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE) //��֤��Ϣ�ύ����������
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
	    }
			POINT_COLOR = BLUE;
			LCD_ShowString(20,100,200,20,16,"authting.....");//��ʾ���յ�������	
			DEBUG("is authing!\n");
		}
		OSTimeDlyHMSM(0,0,1,0);//����������˯��2s
	}
}

//tcp�ͻ��˷�����Ϣ������
void tcp_client_recv(void *arg)
{
	u8 * auth_msg;
	u8 * recv_msg;
	err_t recv_err;
	u32 data_len = 0;
	OS_CPU_SR cpu_sr;
	struct netbuf *recvbuf;
	struct pbuf *q;
	DEBUG("tcp client recv task\r\n");
	while(1)
	{
		while(!isConnected)
		{
			OSTimeDlyHMSM(0,0,2,0);//�ȴ����Ӻ�
		}
		if((recv_err = netconn_recv(tcp_clientconn,&recvbuf)) == ERR_OK)  //���յ�����
		{	
			OS_ENTER_CRITICAL(); //���ж�
			recv_msg = mymalloc(SRAMEX, TCP_RX_BUFSIZE);//����յ�����Ϣ
			for(q=recvbuf->p;q!=NULL;q=q->next)  //����������pbuf����
			{
					//�ж�Ҫ������TCP_RX_BUFSIZE�е������Ƿ����TCP_RX_BUFSIZE��ʣ��ռ䣬�������
					//�Ļ���ֻ����TCP_RX_BUFSIZE��ʣ�೤�ȵ����ݣ�����Ļ��Ϳ������е�����
					if(q->len > (TCP_RX_BUFSIZE-data_len))
					{
						memcpy(recv_msg+data_len,q->payload,(TCP_RX_BUFSIZE-data_len));//��������
					}
					else 
					{
						memcpy(recv_msg+data_len,q->payload,q->len);
					}
					data_len += q->len;  	
					if(data_len > TCP_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
			}
			OS_EXIT_CRITICAL();  //���ж�
			data_len=0;  				//������ɺ�data_lenҪ���㡣					
			netbuf_delete(recvbuf);
			/*�����յ�����Ϣ,ת������������*/
			auth_msg = mymalloc(SRAMEX, 7);
			sprintf((char *)auth_msg, "client");
			if(OSQPost(tcp_handle_event, auth_msg) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
			}		
			if(OSQPost(tcp_handle_event, recv_msg) != OS_ERR_NONE) 
			{
					DEBUG("tcp_client_recv��OSQPost ERROR %d\r\n", __LINE__);
	    }
		}
		else if(recv_err == ERR_CLSD)  //�ر�����
		{
				netconn_close(tcp_clientconn);
				netconn_delete(tcp_clientconn);
				DEBUG("������%d.%d.%d.%d�Ͽ�����\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
				isConnected = 0; //�Ͽ�����
				isAuthed = 0;    //�����֤ʧЧ
				continue;
		}//end of netconn_recv	
	}
}



/**
 * @Function: void handle_message_task(void *arg)
 * @Description: deal with messeages from server and do some actions
 * @Input : NULL
 * @Return: NULL
 */
void tcp_handle_task(void *arg)
{
	u8 type;
	u8 subtype;
	u8 res;
	u8 i;
	u8 j;
	u8 *msg_buf;
	u8 *auth_msg;
	u8 *recv_msg;
	u8 *send_buf;
	INT8U err;
	char account[ACCOUNT_MAX + 1];
	DEBUG("tcp client handle message task\r\n");
	while(1)
	{
		DEBUG("handle message task!\n");
		auth_msg = (u8 *)OSQPend (tcp_handle_event,  //�ȴ��������� or �ͻ��� ��Ϣ
                0,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			 DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
		}
		recv_msg = (u8 *)OSQPend (tcp_handle_event,  //�ȴ���Ϣ
                1000,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			 DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
		}
		if(strcmp((char *)auth_msg, "client") == 0)//�ǿͻ���
		{
			
			i = 0;
			/*�������յ�����Ϣ(���ܰ������ָ��)*/
			while((recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE))
			{
				if((recv_msg[i]!='\0') && recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϊtype
				{
					type = recv_msg[i];
				}
				else
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i += 2;
				/*��¼�˺�*/
				for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
				{
					account[j] = recv_msg[i];
				}
				i++;
				account[j] = '\0';
				if((strcmp(account, master) != 0)&&(strcmp(account, slave) != 0)) //�ų���SLAVE��MASTER����Ϣ
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				/*���������*/
				if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϊtype
				{
					subtype = recv_msg[i];
				}
				else
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i+=2;
				if(type == COMMAND_RESULT)//Ϊ���ָ��
				{
					if(subtype == RES_LOGIN)
					{
						/*�ж�ָ��Ϸ��ԣ����Ҽ���Ƿ�һ��ָ���ĩβEND*/
						if((recv_msg[i+1] == COMMAND_SEPERATOR)&&(recv_msg[i+2] == COMMAND_END))
						{
							res = recv_msg[i];
							if(res == LOGIN_SUCCESS)
							{
								isAuthed = 1;
								POINT_COLOR = GREEN;
								LCD_ShowString(20,120,200,20,16,"Login Success");//��ʾ���յ�������	
							}
							else
							{
								POINT_COLOR = GREEN;
								LCD_ShowString(20,120,200,20,16,"Login Failed ");//��ʾ���յ�������
								isAuthed = 0;
							}
							i+=3;
							continue;//ѭ��������һ��ָ��
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
                i++;
							}
							i++;
							continue;
						}
					}//end of RES_LOGIN
				}//end of command_result
				else if(type == COMMAND_CONTRL)
				{
					if(subtype == CTL_IHome)
					{
						/*��ÿ������ǹر�IHome Mode*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = recv_msg[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						if(res == IHome_START)//start ihome
						{
							ihome_start_flag = IHome_START;
						}
						else if(res == IHome_STOP)//stop ihome
						{
							ihome_start_flag = IHome_STOP;
						}
						/*-------------����IHome mode״̬��Ϣ���û�-----------*/
						send_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)send_buf, "client");
						if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}				
						send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
						sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
																		RES_IHome,COMMAND_SEPERATOR,
																		ihome_start_flag,COMMAND_SEPERATOR,COMMAND_END);
						if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}

					}//end of ctl_ihome
					else if(subtype == CTL_GET)
					{
						/*��������豸��ID*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = recv_msg[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*����豸ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*��ʱ��account�淵��ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						//printf("device ID:%s\n", account);
						if(res == RES_TEMP)
						{
							/*--------------����TEMP��DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "client");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							msg_buf = mymalloc(SRAMEX, 5);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, "TEMP");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
							/*--------------�����¶ȴ�����ID��DHT11����-----------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
						}//end of temp
						else if(res == RES_HUMI)
						{
							/*--------------����HUMI��DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "client");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							msg_buf = mymalloc(SRAMEX, 5);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, "HUMI");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
							/*---------------����ʪ�ȴ�����ID��DHT11����------------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
						}
						
					}//end of ctl_get
					else if(subtype == CTL_LAMP)
					{
						/*��õƿ����ǹ�*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = recv_msg[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*��õ�ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*��ʱ��account�淵��ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						DEBUG("ID��%s \n", account);
						/*��ʾ���յ�������������Ϣ*/
						msg_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)msg_buf, "client");
						if(OSQPost(led_event, msg_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
						{
							 DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}	
						/*����ON OFF��LED TSAK */
						msg_buf = mymalloc(SRAMEX, 10);//�ⲿ�ڴ����ռ�
						if(res == LAMP_ON)
						{
							DEBUG("LAMP_ON\n");
							sprintf((char *)msg_buf, "ON");
						}
						else if(res == LAMP_OFF)
						{
							DEBUG("LAMP_OFF\n");
							sprintf((char *)msg_buf, "OFF");
						}
						if(OSQPost(led_event,msg_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}
						/*���͵�ID��LED����*/
						msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//�ⲿ�ڴ����ռ�
						sprintf((char *)msg_buf, (char *)account);
						if(OSQPost(led_event,msg_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}
					}//end of ctl_lamp
				
				}//end of command contrl
			}//end of while(msg)
			
		}//end of client;
		else if(strcmp((char *)auth_msg, "server") == 0)//������Ϊ��������Ϣ
		{
			
			i = 0;
			/*�������յ�����Ϣ(���ܰ������ָ��)*/
			while((recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE))
			{
				if((recv_msg[i]!='\0') && recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϊtype
				{
					type = recv_msg[i];
				}
				else
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i += 2;
				/*��¼�˺�*/
				for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
				{
					account[j] = recv_msg[i];
				}
				i++;
				account[j] = '\0';
				if((strcmp(account, master) != 0)&&(strcmp(account, slave) != 0)) //�ų���SLAVE��MASTER����Ϣ
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				/*���������*/
				if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϊtype
				{
					subtype = recv_msg[i];
				}
				else
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i+=2;
				if(type == COMMAND_MANAGE)//��¼ָ��
				{
					if(subtype == MAN_LOGIN)
					{
						if(strcmp(account, master) == 0)//���յ��û���������������
						{
							/*��ʱ��account��¼����*/
							for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
							{
								account[j] = recv_msg[i];
							}
							i++;
							account[j] = '\0';
							
							/*-------------return msg to user-----------*/
							send_buf = mymalloc(SRAMEX, 7);
						  sprintf((char *)send_buf, "server");
							if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
							{
									DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							if(strcmp(account, password) == 0)//��½�ɹ�
							{
									/*-------------return msg about login_success to user-----------*/		
									send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
									sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,slave,COMMAND_SEPERATOR,
																		RES_LOGIN,COMMAND_SEPERATOR,
																		LOGIN_SUCCESS,COMMAND_SEPERATOR,COMMAND_END);
									if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
									{
										DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
									}
							}//end of LOGIN SUCCESS
							else//LOGIN_FAILED 
							{
									/*-------------return msg about login_failed to user-----------*/		
									send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
									sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,slave,COMMAND_SEPERATOR,
																		RES_LOGIN,COMMAND_SEPERATOR,
																		LOGIN_FAILED,COMMAND_SEPERATOR,COMMAND_END);
									if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
									{
										DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
									}
							}
							
						}
					}//end of RES_LOGIN
				}//end of command_result
				else if(type == COMMAND_CONTRL)
				{
					if(subtype == CTL_IHome)
					{
						/*��ÿ������ǹر�IHome Mode*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = recv_msg[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						if(res == IHome_START)//start ihome
						{
							ihome_start_flag = IHome_START;
						}
						else if(res == IHome_STOP)//stop ihome
						{
							ihome_start_flag = IHome_STOP;
						}
						/*-------------����IHome mode״̬��Ϣ���û�-----------*/
						send_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)send_buf, "server");
						if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}				
						send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
						sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,slave,COMMAND_SEPERATOR,
																		RES_IHome,COMMAND_SEPERATOR,
																		ihome_start_flag,COMMAND_SEPERATOR,COMMAND_END);
						if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}

					}//end of ctl_ihome
					else if(subtype == CTL_GET)
					{
						/*��������豸��ID*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = recv_msg[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*����豸ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*��ʱ��account�淵��ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						//printf("device ID:%s\n", account);
						if(res == RES_TEMP)
						{
							/*--------------����TEMP��DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "server");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							msg_buf = mymalloc(SRAMEX, 5);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, "TEMP");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
							/*--------------�����¶ȴ�����ID��DHT11����-----------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
						}//end of temp
						else if(res == RES_HUMI)
						{
							/*--------------����HUMI��DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "server");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							msg_buf = mymalloc(SRAMEX, 5);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, "HUMI");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
							/*---------------����ʪ�ȴ�����ID��DHT11����------------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
						}
						
					}//end of ctl_get
					else if(subtype == CTL_LAMP)
					{
						/*��õƿ����ǹ�*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = recv_msg[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*��õ�ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*��ʱ��account�淵��ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						DEBUG("ID��%s \n", account);
						/*��ʾ���յ�������������Ϣ*/
						msg_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)msg_buf, "server");
						if(OSQPost(led_event, msg_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
						{
							 DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}	
						/*����ON OFF��LED TSAK */
						msg_buf = mymalloc(SRAMEX, 10);//�ⲿ�ڴ����ռ�
						if(res == LAMP_ON)
						{
							DEBUG("LAMP_ON\n");
							sprintf((char *)msg_buf, "ON");
						}
						else if(res == LAMP_OFF)
						{
							DEBUG("LAMP_OFF\n");
							sprintf((char *)msg_buf, "OFF");
						}
						if(OSQPost(led_event,msg_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}
						/*���͵�ID��LED����*/
						msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//�ⲿ�ڴ����ռ�
						sprintf((char *)msg_buf, (char *)account);
						if(OSQPost(led_event,msg_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}
					}//end of ctl_lamp
				
				}//end of command contrl
			}//end of while(msg)
			
		}//end of server;
		
		
	}// end of while(1)
}

/**
 * @Function: void tcp_send_task(void *arg)
 * @Description: 
 *					as "client": send message to server
 *					as "server": send message to user
 */
void tcp_send_task(void *arg)
{
	u8 *auth_msg; //�����Ϣ
	u8 *msg;      //���յ�����Ϣ
	INT8U err;
	DEBUG("tcp send task\r\n");
	while(1)
	{
		while(!isConnected)
		{
			OSTimeDlyHMSM(0,0,2,0);//�ȴ����Ӻ�
		}
		auth_msg = (u8 *)OSQPend (tcp_send_event,  //�ȴ�����Ϣ
                0,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("rev auth_msg err! %s %d\n", __FILE__, __LINE__);
			myfree(SRAMEX, auth_msg);
			continue; //��һ��ѭ��
		}
		msg = (u8 *)OSQPend (tcp_send_event,  //�ȴ�����Ϣ
                2000,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			 DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
		}
		else
		{
			if(strcmp((char *)auth_msg, "client") == 0)//�ͻ��˷��͸�������
			{
					err = netconn_write(tcp_clientconn ,msg,strlen((char*)msg),NETCONN_COPY); //����tcp_server_sentbuf�е�����
					if(err != ERR_OK)
					{
						DEBUG("����ʧ��\r\n");
						isConnected = 0;
					}
			}
			else if(strcmp((char *)auth_msg, "server") == 0)//��Ϊ���������͸��û�
			{
					err = netconn_write(serverconn ,msg,strlen((char*)msg),NETCONN_COPY); //����tcp_server_sendbuf�е�����
					if(err != ERR_OK)
					{
						DEBUG("����ʧ��\r\n");
						isAccepted = 0;
					}
			}
			

			myfree(SRAMEX, msg); //�ͷ���Ϣ�Ŀռ䣬��ֹ�ڴ�й¶
		}
		myfree(SRAMEX, auth_msg);
	}
	
}

