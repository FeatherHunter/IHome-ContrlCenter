#include "tcp_client.h" 
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "malloc.h"
#include "stdio.h"
#include "string.h" 
#include "task_priority.h"
/**
 * @copyright ������(2015~2025) QQ��975559549
 * @Author Feather @version 1.0 @date 2015.12.30
 * @filename tcp_client.c
 * @description including tcp connect task, handle with msg that recv
 * @FunctionList
 *		1.void tcp_connect_task(void *arg);   //tcp���Ӻͱ��������Լ������֤������
 *    2.u8 KEY_Scan(u8 mode);
 */ 

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
	printf("tcp connect task!\n");
	tcp_client_sendbuf = mymalloc(SRAMEX, TCP_CLIENT_TX_BUFSIZE);
	if(tcp_client_sendbuf==NULL)
	{
		printf("tcp_client_sendbuf malloc error!,%s %d\n", __FILE__, __LINE__);
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
		 printf("tcp_client_pcb null,%s %d\n", __FILE__, __LINE__);
	}
	while(1)
	{
		LED1 = !LED1;
		isConnected = tcp_client_flag&1<<5;
		while(isConnected && isAuthed)
		{
			printf("tcp sleep!\n");
			sprintf(tcp_client_sendbuf, "%c%c%c",COMMAND_PULSE,COMMAND_SEPERATOR,COMMAND_END);
			tcp_client_flag|=1<<7;//���Ҫ��������
			pulse = 0; //2����pulse��ˢ�£���ʾ���յ�����
			OSTimeDlyHMSM(0,0,2,0);
			if(pulse == 0)//�Ͽ�����
			{
				tcp_client_flag &= ~(1<<5);
				isConnected = 0;
				isAuthed = 0;
			}
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
				printf("tcp_client_pcb = null\n");
			}
			isConnected = tcp_client_flag&1<<5;
			isAuthed = 0;
			printf("is connecting!\n");
		}
		/*--------��֤��Ϣ---------*/
		if((isConnected != 0)&&(isAuthed == 0))
		{
			sprintf(tcp_client_sendbuf, "%c%c975559549h%c%c%c975559549%c%c",COMMAND_MANAGE,COMMAND_SEPERATOR,COMMAND_SEPERATOR,MAN_LOGIN,COMMAND_SEPERATOR,COMMAND_SEPERATOR, COMMAND_END);
			tcp_client_flag|=1<<7;//���Ҫ��������
			printf("is authing!\n");
		}
		OSTimeDlyHMSM(0,0,0,1000);
		printf("tcp connect task!\n");
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
	char account[ACCOUNT_MAX + 1];
	printf("handle message task!\n");
	while(1)
	{
		while(isConnected == 0)
		{
			OSTimeDlyHMSM(0,0,2,0);//û�����Ӻã��ȴ�2S����
		}
		if(tcp_client_flag&1<<6)//�Ƿ��յ�����?
		{
			printf("handle message task!\n");
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
				if(strcmp(account, "975559549") != 0) //���Ǹû���������
				{
					break;
				}
				/*�˻���֤�ɹ�,���������*/
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
				}
			}
			
			LCD_ShowString(30,250,lcddev.width-30,lcddev.height-230,16,tcp_client_recvbuf);//��ʾ���յ�������		
			tcp_client_flag&=~(1<<6);//��������Ѿ���������.
		}
		
	}
}

//TCP Client ����
void tcp_client_test(void)
{
 	struct tcp_pcb *tcppcb;  	//����һ��TCP���������ƿ�
	struct ip_addr rmtipaddr;  	//Զ��ip��ַ
	
	u8 *tbuf;
 	u8 key;
	u8 res=0;		
	u8 t=0; 
	
	//tcp_client_set_remoteip();//��ѡ��IP
	lwipdev.remoteip[0]=server_ip[0];
	lwipdev.remoteip[1]=server_ip[1];
	lwipdev.remoteip[2]=server_ip[2]; 
	lwipdev.remoteip[3]=server_ip[3]; 
	
	LCD_Clear(WHITE);	//����
	POINT_COLOR=RED; 	//��ɫ����
	LCD_ShowString(30,30,200,16,16,"Explorer STM32F4");
	LCD_ShowString(30,50,200,16,16,"TCP Client Test");
	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");  
	LCD_ShowString(30,90,200,16,16,"KEY0:Send data");  
	LCD_ShowString(30,110,200,16,16,"KEY_UP:Quit");  
	LCD_ShowString(30,130,200,16,16,"When break,please quit!");  
	tbuf=mymalloc(SRAMIN,200);	//�����ڴ�
	if(tbuf==NULL)return ;		//�ڴ�����ʧ����,ֱ���˳�
	sprintf((char*)tbuf,"Local IP:%d.%d.%d.%d",lwipdev.ip[0],lwipdev.ip[1],lwipdev.ip[2],lwipdev.ip[3]);//������IP
	LCD_ShowString(30,150,210,16,16,tbuf);  
	sprintf((char*)tbuf,"Remote IP:%d.%d.%d.%d",lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]);//Զ��IP
	LCD_ShowString(30,170,210,16,16,tbuf);  
	sprintf((char*)tbuf,"Remotewo Port:%d",TCP_CLIENT_PORT);//�ͻ��˶˿ں�
	LCD_ShowString(30,190,210,16,16,tbuf);
	POINT_COLOR=BLUE;
	LCD_ShowString(30,210,210,16,16,"STATUS:Disconnected"); 
	tcppcb=tcp_new();	//����һ���µ�pcb
	if(tcppcb)			//�����ɹ�
	{
		IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]); 
		tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);  //���ӵ�Ŀ�ĵ�ַ��ָ���˿���,�����ӳɹ���ص�tcp_client_connected()����
 	}else res=1;
	while(res==0)
	{
		key=KEY_Scan(0);
		if(key==WKUP_PRES)break;
		if(key==KEY0_PRES)//KEY0������,��������
		{
			tcp_client_flag|=1<<7;//���Ҫ��������
		}
		if(tcp_client_flag&1<<6)//�Ƿ��յ�����?
		{
			LCD_Fill(30,250,lcddev.width-1,lcddev.height-1,WHITE);//����һ������
			LCD_ShowString(30,250,lcddev.width-30,lcddev.height-230,16,tcp_client_recvbuf);//��ʾ���յ�������		
			tcp_client_flag&=~(1<<6);//��������Ѿ���������.
		}
		if(tcp_client_flag&1<<5)//�Ƿ�������?
		{
			LCD_ShowString(30,210,lcddev.width-30,lcddev.height-190,16,"STATUS:Connected   ");//��ʾ��Ϣ		
			POINT_COLOR=RED;
			LCD_ShowString(30,230,lcddev.width-30,lcddev.height-190,16,"Receive Data:");//��ʾ��Ϣ		
			POINT_COLOR=BLUE;//��ɫ����
		}else if((tcp_client_flag&1<<5)==0)
		{
 			LCD_ShowString(30,210,190,16,16,"STATUS:Disconnected");
			LCD_Fill(30,230,lcddev.width-1,lcddev.height-1,WHITE);//����
		} 
		lwip_periodic_handle();
		delay_ms(2);
		t++;
		if(t==200)
		{
			if((tcp_client_flag&1<<5)==0)//δ������,��������
			{ 
				tcp_client_connection_close(tcppcb,0);//�ر�����
				tcppcb=tcp_new();	//����һ���µ�pcb
				if(tcppcb)			//�����ɹ�
				{ 
					tcp_connect(tcppcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);//���ӵ�Ŀ�ĵ�ַ��ָ���˿���,�����ӳɹ���ص�tcp_client_connected()����
				}
			}
			t=0;
			LED0=!LED0;
		}		
	}
	tcp_client_connection_close(tcppcb,0);//�ر�TCP Client����
	LCD_Clear(WHITE);
	POINT_COLOR = RED;
	LCD_ShowString(30,30,200,16,16,"Explorer STM32F4");
	LCD_ShowString(30,50,200,16,16,"TCP Client Test"); 
	LCD_ShowString(30,70,200,16,16,"ATOM@ALIENTEK");
	
	POINT_COLOR=BLUE;
	LCD_ShowString(30,90,200,16,16,"Connect break��");  
	LCD_ShowString(30,110,200,16,16,"KEY1:Connect");
	myfree(SRAMIN,tbuf);
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
	err_t ret_err;
	struct tcp_client_struct *es; 
	es=(struct tcp_client_struct*)arg;
	if(es!=NULL)  //���Ӵ��ڿ��п��Է�������
	{
		if(tcp_client_flag&(1<<7))	//�ж��Ƿ�������Ҫ���� 
		{
			es->p=pbuf_alloc(PBUF_TRANSPORT, strlen((char*)tcp_client_sendbuf),PBUF_POOL);	//�����ڴ� 
			pbuf_take(es->p,(char*)tcp_client_sendbuf,strlen((char*)tcp_client_sendbuf));	//��tcp_client_sentbuf[]�е����ݿ�����es->p_tx��
			tcp_client_senddata(tpcb,es);//��tcp_client_sentbuf[]���渴�Ƹ�pbuf�����ݷ��ͳ�ȥ
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



