#include "tcp_client.h" 
#include "task_priority.h"
#include "ihome_function.h"
#include "idebug.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "malloc.h"
#include "stdio.h"
#include "string.h" 
/**
 * @copyright ������(2015~2025) QQ��975559549
 * @Author Feather @version 1.0 @date 2015.12.30
 * @filename tcp_client.c
 * @description including tcp connect task, handle with msg that recv
 * @FunctionList
 *		1.void tcp_connect_task(void *arg);   //tcp���Ӻͱ��������Լ������֤������
 *    2.u8 KEY_Scan(u8 mode);
 */ 

/*send��Ϣ����*/
void *send_q[SENDSIZE];
OS_EVENT * msg_event;

int pulse = 1;
 
/*  tcp ���������ջ         */
OS_STK TCP_CONNECT_TASK_STK[TCP_CONNECT_STK_SIZE];	
/*  ����ӷ��������ܵ�����Ϣ ��ջ*/
OS_STK HANDLE_MSG_TASK_STK[HANDLE_MSG_STK_SIZE];	

struct tcp_pcb *tcp_client_pcb;  	//����һ��TCP���������ƿ�
struct ip_addr rmtipaddr;  	//Զ��ip��ַ
/*��־*/
u8 isConnected = 0;
u8 isAuthed = 0;
/*TCP Client�������ݻ����������ͻ�����*/
u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	
u8 *tcp_client_sendbuf;
/*ip��ַ*/
u8 server_ip[4] = {139, 129, 19, 115};

/*TCP Client ����ȫ��״̬��Ǳ���
 *bit7:0,û������Ҫ����;1,������Ҫ����
 *bit6:0,û���յ�����;1,�յ�������.
 *bit5:0,û�������Ϸ�����;1,�����Ϸ�������.
 *bit4~0:����
 */
u8 tcp_client_flag = 0;	 

/**
 * @Function: void tcp_connect_task(void *arg)
 * @Description: keep connecting with server and make authetication
 * @Input : NULL
 * @Return: NULL
 */
void tcp_connect_task(void *arg)
{
	u8 * send_buf;
	DEBUG("tcp connect task!\n");
	tcp_client_sendbuf = mymalloc(SRAMEX, TCP_CLIENT_TX_BUFSIZE);
	if(tcp_client_sendbuf==NULL)
	{
		DEBUG("tcp_client_sendbuf malloc error!,%s %d\n", __FILE__, __LINE__);
	}
	/*���÷�����ip��ַ*/
	lwipdev.remoteip[0]=server_ip[0];
	lwipdev.remoteip[1]=server_ip[1];
	lwipdev.remoteip[2]=server_ip[2]; 
	lwipdev.remoteip[3]=server_ip[3];
	
	tcp_client_pcb=tcp_new();	//����һ���µ�pcb
	if(tcp_client_pcb)			  //�����ɹ�
	{ 
			IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]); //��IP��ַ
			tcp_connect(tcp_client_pcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);//���ӵ�Ŀ�ĵ�ַ��ָ���˿���,�����ӳɹ���ص�tcp_client_connected()����
  }
	else
	{
		 DEBUG("tcp_client_pcb null,%s %d\n", __FILE__, __LINE__);
	}
	while(1)
	{
		LED1 = !LED1;
		isConnected = tcp_client_flag&1<<5;
		while(isConnected && isAuthed)
		{
// 			printf("tcp sleep!\n");
// 			/*����ָ��*/
// 			send_buf = mymalloc(SRAMEX, 4); //4���͹���
// 			sprintf((char *)send_buf, "%c%c%c",COMMAND_PULSE,COMMAND_SEPERATOR,COMMAND_END);
// 			if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
// 			{
// 					printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
// 	    }
// 			tcp_client_flag|=1<<7;//���Ҫ��������
// 			
// 			pulse = 0; //5����pulse��ˢ�£���ʾ���յ�����
			OSTimeDlyHMSM(0,0,5,0);
// 			if(pulse == 0)//�Ͽ�����
// 			{
// 				tcp_client_flag &= ~(1<<5);
// 				isConnected = 0;
// 				isAuthed = 0;
// 			}
		}
		/*--------��������---------*/
		if(isConnected == 0)
		{
			tcp_client_connection_close(tcp_client_pcb,0);//�ر�����
			tcp_client_pcb=tcp_new();	//����һ���µ�pcb
			if(tcp_client_pcb)			//�����ɹ�
			{ 
				tcp_connect(tcp_client_pcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);//���ӵ�Ŀ�ĵ�ַ��ָ���˿���,�����ӳɹ���ص�tcp_client_connected()����
			}
			else
			{
				DEBUG("tcp_client_pcb = null\n");
			}
			isConnected = tcp_client_flag&1<<5;
			isAuthed = 0;
			DEBUG("is connecting!\n");
		}
		/*--------��֤��Ϣ---------*/
		if((isConnected != 0)&&(isAuthed == 0))
		{
			/*�������ָ��*/
			send_buf = mymalloc(SRAMEX, 73); //32(account)+32(password)+7+1 ��֤��Ϣ�������
			sprintf((char *)send_buf, "%c%c975559549h%c%c%c975559549%c%c",COMMAND_MANAGE,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR,
																			MAN_LOGIN,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR, COMMAND_END);
			if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
	    }
			tcp_client_flag|=1<<7;//���Ҫ��������
			
			DEBUG("is authing!\n");
		}
		OSTimeDlyHMSM(0,0,0,1000);
		DEBUG("tcp connect task!\n");
	}

}

/**
 * @Function: void handle_message_task(void *arg)
 * @Description: deal with messeages from server and do some actions
 * @Input : NULL
 * @Return: NULL
 */
void handle_message_task(void *arg)
{
	u8 type;
	u8 subtype;
	u8 res;
	u8 i;
	u8 j;
	u8 *msg_buf;
	u8 *send_buf;
	char account[ACCOUNT_MAX + 1];
	DEBUG("handle message task!\n");
	while(1)
	{
		while(isConnected == 0)
		{
			OSTimeDlyHMSM(0,0,2,0);//û�����Ӻã��ȴ�2S����
		}
		if(tcp_client_flag&(1<<6))//�Ƿ��յ�����?
		{
			DEBUG("handle message task!\n");
			LCD_ShowString(30,250,lcddev.width-30,lcddev.height-230,16,tcp_client_recvbuf);//��ʾ���յ�������
			i = 0;
			/*�������յ�����Ϣ(���ܰ������ָ��)*/
			while((tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE))
			{
				if((tcp_client_recvbuf[i]!='\0') && tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϊtype
				{
					type = tcp_client_recvbuf[i];
					/*����*/
          if(type == COMMAND_PULSE)
          {
						pulse = 1;
						  
            while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
            {
                 i++;
            }
            i++;
            continue;
          }
				}
				else
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i += 2;
				/*��¼�˺�*/
				for(j = 0; (tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE)&&(tcp_client_recvbuf[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
				{
					account[j] = tcp_client_recvbuf[i];
				}
				i++;
				account[j] = '\0';
				if((strcmp(account, MASTER) != 0)&&(strcmp(account, SLAVE) != 0)) //�ų���SLAVE��MASTER����Ϣ
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				/*���������*/
				if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϊtype
				{
					subtype = tcp_client_recvbuf[i];
				}
				else
				{
					/*��ǰָ����Ч,��ת����һ��ָ��*/
					while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
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
						if((tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)&&(tcp_client_recvbuf[i+2] == COMMAND_END))
						{
							res = tcp_client_recvbuf[i];
							if(res == LOGIN_SUCCESS)
							{
								isAuthed = 1;
								LCD_ShowString(30,270,lcddev.width-30,lcddev.height-230,16,"LOGIN SUCCESS");//��ʾ���յ�������	
							}
							else
							{
								isAuthed = 0;
							}
							i+=3;
							continue;//ѭ��������һ��ָ��
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
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
						if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = tcp_client_recvbuf[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
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
						send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 ״ָ̬���������
						sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
																		RES_IHome,COMMAND_SEPERATOR,
																		ihome_start_flag,COMMAND_SEPERATOR,COMMAND_END);
						if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}
						tcp_client_flag|=1<<7;//���Ҫ��������
					}//end of ctl_ihome
					else if(subtype == CTL_GET)
					{
						/*��������豸��ID*/
						if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = tcp_client_recvbuf[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*����豸ID*/
						for(j = 0; (tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE)&&(tcp_client_recvbuf[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*��ʱ��account�淵��ID*/
							account[j] = tcp_client_recvbuf[i];
						}
						i++;
						account[j] = '\0';
						//printf("device ID:%s\n", account);
						if(res == RES_TEMP)
						{
							/*--------------����TEMP��DHT11 TSAK------------------------------*/
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
							msg_buf = mymalloc(SRAMEX, 5);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, "HUMI");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
							/*---------------����ʪ�ȴ�����ID��DHT11����------------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//�ⲿ�ڴ����ռ�
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
						}
						
					}//end of ctl_get
					else if(subtype == CTL_LAMP)
					{
						/*��õƿ����ǹ�*/
						if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//�ж��Ƿ�Ϸ�
						{
							res = tcp_client_recvbuf[i];
						}
						else
						{
							/*��ǰָ����Ч,��ת����һ��ָ��*/
							while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*��õ�ID*/
						for(j = 0; (tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE)&&(tcp_client_recvbuf[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*��ʱ��account�淵��ID*/
							account[j] = tcp_client_recvbuf[i];
						}
						i++;
						account[j] = '\0';
						DEBUG("ID��%s \n", account);
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
			}
			
			LCD_ShowString(30,250,lcddev.width-30,lcddev.height-230,16,tcp_client_recvbuf);//��ʾ���յ�������
			
			tcp_client_flag&=~(1<<6);//��������Ѿ���������.
		}
	  //lwip_periodic_handle();//LWIP��ѯ����
  }// end of while(1)
}

//lwIP TCP���ӽ�������ûص�����
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	struct tcp_client_struct *es=NULL;  
	if(err==ERR_OK)   
	{
		es=(struct tcp_client_struct*)mem_malloc(sizeof(struct tcp_client_struct));  //�����ڴ�
		if(es) //�ڴ�����ɹ�
		{
 			es->state=ES_TCPCLIENT_CONNECTED;//״̬Ϊ���ӳɹ�
			es->pcb=tpcb;  
			es->p=NULL; 
			tcp_arg(tpcb,es);        			    //ʹ��es����tpcb��callback_arg
			tcp_recv(tpcb,tcp_client_recv);  	//��ʼ��LwIP��tcp_recv�ص�����   
			tcp_err(tpcb,tcp_client_error); 	//��ʼ��tcp_err()�ص�����
			tcp_sent(tpcb,tcp_client_sent);		//��ʼ��LwIP��tcp_sent�ص�����
			tcp_poll(tpcb,tcp_client_poll,1); //��ʼ��LwIP��tcp_poll�ص����� 
 			tcp_client_flag|=1<<5; 				    //������ӵ���������
			err=ERR_OK;
		}else
		{ 
			tcp_client_connection_close(tpcb,es);//�ر�����
			err=ERR_MEM;	//�����ڴ�������
		}
	}else
	{
		tcp_client_connection_close(tpcb,0);//�ر�����
	}
	return err;
}
    
//lwIP tcp_recv()�����Ļص�����
err_t tcp_client_recv(void *arg,struct tcp_pcb *tpcb,struct pbuf *p,err_t err)
{ 
	u32 data_len = 0;
	struct pbuf *q;
	struct tcp_client_struct *es;
	err_t ret_err; 
	LWIP_ASSERT("arg != NULL",arg != NULL);
	es=(struct tcp_client_struct *)arg; 
	if(p==NULL)//����ӷ��������յ��յ�����֡�͹ر�����
	{
		es->state=ES_TCPCLIENT_CLOSING;//��Ҫ�ر�TCP ������ 
 		es->p=p; 
		ret_err=ERR_OK;
	}else if(err!= ERR_OK)//�����յ�һ���ǿյ�����֡,����err!=ERR_OK
	{ 
		if(p)pbuf_free(p);//�ͷŽ���pbuf
		ret_err=err;
	}else if(es->state==ES_TCPCLIENT_CONNECTED)	//����������״̬ʱ
	{
		if(p!=NULL)//����������״̬���ҽ��յ������ݲ�Ϊ��ʱ
		{
			memset(tcp_client_recvbuf,0,TCP_CLIENT_RX_BUFSIZE);  //���ݽ��ջ���������
			for(q=p;q!=NULL;q=q->next)  //����������pbuf����
			{
				//�ж�Ҫ������TCP_CLIENT_RX_BUFSIZE�е������Ƿ����TCP_CLIENT_RX_BUFSIZE��ʣ��ռ䣬�������
				//�Ļ���ֻ����TCP_CLIENT_RX_BUFSIZE��ʣ�೤�ȵ����ݣ�����Ļ��Ϳ������е�����
				if(q->len > (TCP_CLIENT_RX_BUFSIZE-data_len)) memcpy(tcp_client_recvbuf+data_len,q->payload,(TCP_CLIENT_RX_BUFSIZE-data_len));//��������
				else memcpy(tcp_client_recvbuf+data_len,q->payload,q->len);
				data_len += q->len;  	
				if(data_len > TCP_CLIENT_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
			}
			tcp_client_flag|=1<<6;		//��ǽ��յ�������
 			tcp_recved(tpcb,p->tot_len);//���ڻ�ȡ��������,֪ͨLWIP���Ի�ȡ��������
			pbuf_free(p);  	//�ͷ��ڴ�
			ret_err=ERR_OK;
		}
	}else  //���յ����ݵ��������Ѿ��ر�,
	{ 
		tcp_recved(tpcb,p->tot_len);//���ڻ�ȡ��������,֪ͨLWIP���Ի�ȡ��������
		es->p=NULL;
		pbuf_free(p); //�ͷ��ڴ�
		ret_err=ERR_OK;
	}
	return ret_err;
}
//lwIP tcp_err�����Ļص�����
void tcp_client_error(void *arg,err_t err)
{  
	//�������ǲ����κδ���
} 

//lwIP tcp_poll�Ļص�����
err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb)
{
	u8 *msg;//���յ�����Ϣ
	INT8U err;
	
	err_t ret_err;
	struct tcp_client_struct *es; 
	es=(struct tcp_client_struct*)arg;
	if(es!=NULL)  //���Ӵ��ڿ��п��Է�������
	{
		if(tcp_client_flag&(1<<7))	//�ж��Ƿ�������Ҫ���� 
		{
			msg = (u8 *)OSQPend (msg_event,  //�ȴ�����Ϣ
                0,  //wait forever
                &err);
			if(err != OS_ERR_NONE)
			{
				DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
			}
			es->p=pbuf_alloc(PBUF_TRANSPORT, strlen((char*)msg),PBUF_POOL);	//�����ڴ� 
			pbuf_take(es->p,(char*)msg,strlen((char*)msg));	//��tcp_client_sentbuf[]�е����ݿ�����es->p_tx��
			tcp_client_senddata(tpcb,es);//��tcp_client_sentbuf[]���渴�Ƹ�pbuf�����ݷ��ͳ�ȥ
			
			myfree(SRAMEX, msg); //����ռ�
			tcp_client_flag&=~(1<<7);	//������ݷ��ͱ�־
			if(es->p)pbuf_free(es->p);	//�ͷ��ڴ�
		}else if(es->state==ES_TCPCLIENT_CLOSING)
		{ 
 			tcp_client_connection_close(tpcb,es);//�ر�TCP����
		} 
		ret_err=ERR_OK;
	}else
	{ 
		tcp_abort(tpcb);//��ֹ����,ɾ��pcb���ƿ�
		ret_err=ERR_ABRT;
	}
	return ret_err;
} 
//lwIP tcp_sent�Ļص�����(����Զ���������յ�ACK�źź�������)
err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct tcp_client_struct *es;
	LWIP_UNUSED_ARG(len);
	es=(struct tcp_client_struct*)arg;
	if(es->p)tcp_client_senddata(tpcb,es);//��������
	return ERR_OK;
}
//�˺���������������
void tcp_client_senddata(struct tcp_pcb *tpcb, struct tcp_client_struct * es)
{
	struct pbuf *ptr; 
 	err_t wr_err=ERR_OK;
	while((wr_err==ERR_OK)&&es->p&&(es->p->len<=tcp_sndbuf(tpcb)))
	{
		ptr=es->p;
		wr_err=tcp_write(tpcb,ptr->payload,ptr->len,1); //��Ҫ���͵����ݼ��뵽���ͻ��������
		if(wr_err==ERR_OK)
		{  
			es->p=ptr->next;			//ָ����һ��pbuf
			if(es->p)pbuf_ref(es->p);	//pbuf��ref��һ
			pbuf_free(ptr);				//�ͷ�ptr 
		}else if(wr_err==ERR_MEM)es->p=ptr;
		tcp_output(tpcb);		//�����ͻ�������е������������ͳ�ȥ
	} 	
} 
//�ر��������������
void tcp_client_connection_close(struct tcp_pcb *tpcb, struct tcp_client_struct * es)
{
	//�Ƴ��ص�
	tcp_abort(tpcb);//��ֹ����,ɾ��pcb���ƿ�
	tcp_arg(tpcb,NULL);  
	tcp_recv(tpcb,NULL);
	tcp_sent(tpcb,NULL);
	tcp_err(tpcb,NULL);
	tcp_poll(tpcb,NULL,0);  
	if(es)mem_free(es); 
	tcp_client_flag&=~(1<<5);//������ӶϿ���
}
