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
 * @copyright 王辰浩(2015~2025) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.12.30
 * @filename tcp_client.c
 * @description including tcp connect task, handle with msg that recv
 * @FunctionList
 *		1.void tcp_connect_task(void *arg);   //tcp链接和保持链接以及身份验证的任务
 *    2.u8 KEY_Scan(u8 mode);
 */ 

/*send消息队列*/
void *send_q[SENDSIZE];
OS_EVENT * msg_event;

int pulse = 1;
 
/*  tcp 连接任务堆栈         */
OS_STK TCP_CONNECT_TASK_STK[TCP_CONNECT_STK_SIZE];	
/*  处理从服务器接受到的信息 堆栈*/
OS_STK HANDLE_MSG_TASK_STK[HANDLE_MSG_STK_SIZE];	

struct tcp_pcb *tcp_client_pcb;  	//定义一个TCP服务器控制块
struct ip_addr rmtipaddr;  	//远端ip地址
/*标志*/
u8 isConnected = 0;
u8 isAuthed = 0;
/*TCP Client接收数据缓冲区，发送缓冲区*/
u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	
u8 *tcp_client_sendbuf;
/*ip地址*/
u8 server_ip[4] = {139, 129, 19, 115};

/*TCP Client 测试全局状态标记变量
 *bit7:0,没有数据要发送;1,有数据要发送
 *bit6:0,没有收到数据;1,收到数据了.
 *bit5:0,没有连接上服务器;1,连接上服务器了.
 *bit4~0:保留
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
	/*设置服务器ip地址*/
	lwipdev.remoteip[0]=server_ip[0];
	lwipdev.remoteip[1]=server_ip[1];
	lwipdev.remoteip[2]=server_ip[2]; 
	lwipdev.remoteip[3]=server_ip[3];
	
	tcp_client_pcb=tcp_new();	//创建一个新的pcb
	if(tcp_client_pcb)			  //创建成功
	{ 
			IP4_ADDR(&rmtipaddr,lwipdev.remoteip[0],lwipdev.remoteip[1],lwipdev.remoteip[2],lwipdev.remoteip[3]); //绑定IP地址
			tcp_connect(tcp_client_pcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);//连接到目的地址的指定端口上,当连接成功后回调tcp_client_connected()函数
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
// 			/*发送指令*/
// 			send_buf = mymalloc(SRAMEX, 4); //4个就够了
// 			sprintf((char *)send_buf, "%c%c%c",COMMAND_PULSE,COMMAND_SEPERATOR,COMMAND_END);
// 			if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
// 			{
// 					printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
// 	    }
// 			tcp_client_flag|=1<<7;//标记要发送数据
// 			
// 			pulse = 0; //5秒内pulse被刷新，表示接收到心跳
			OSTimeDlyHMSM(0,0,5,0);
// 			if(pulse == 0)//断开连接
// 			{
// 				tcp_client_flag &= ~(1<<5);
// 				isConnected = 0;
// 				isAuthed = 0;
// 			}
		}
		/*--------重新连接---------*/
		if(isConnected == 0)
		{
			tcp_client_connection_close(tcp_client_pcb,0);//关闭连接
			tcp_client_pcb=tcp_new();	//创建一个新的pcb
			if(tcp_client_pcb)			//创建成功
			{ 
				tcp_connect(tcp_client_pcb,&rmtipaddr,TCP_CLIENT_PORT,tcp_client_connected);//连接到目的地址的指定端口上,当连接成功后回调tcp_client_connected()函数
			}
			else
			{
				DEBUG("tcp_client_pcb = null\n");
			}
			isConnected = tcp_client_flag&1<<5;
			isAuthed = 0;
			DEBUG("is connecting!\n");
		}
		/*--------认证信息---------*/
		if((isConnected != 0)&&(isAuthed == 0))
		{
			/*发送身份指令*/
			send_buf = mymalloc(SRAMEX, 73); //32(account)+32(password)+7+1 认证信息最高上限
			sprintf((char *)send_buf, "%c%c975559549h%c%c%c975559549%c%c",COMMAND_MANAGE,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR,
																			MAN_LOGIN,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR, COMMAND_END);
			if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
	    }
			tcp_client_flag|=1<<7;//标记要发送数据
			
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
			OSTimeDlyHMSM(0,0,2,0);//没有连接好，等待2S链接
		}
		if(tcp_client_flag&(1<<6))//是否收到数据?
		{
			DEBUG("handle message task!\n");
			LCD_ShowString(30,250,lcddev.width-30,lcddev.height-230,16,tcp_client_recvbuf);//显示接收到的数据
			i = 0;
			/*解析接收到的信息(可能包含多个指令)*/
			while((tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE))
			{
				if((tcp_client_recvbuf[i]!='\0') && tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//判断是否为type
				{
					type = tcp_client_recvbuf[i];
					/*心跳*/
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
					/*当前指令无效,跳转到下一个指令*/
					while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i += 2;
				/*记录账号*/
				for(j = 0; (tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE)&&(tcp_client_recvbuf[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
				{
					account[j] = tcp_client_recvbuf[i];
				}
				i++;
				account[j] = '\0';
				if((strcmp(account, MASTER) != 0)&&(strcmp(account, SLAVE) != 0)) //排除非SLAVE和MASTER的信息
				{
					/*当前指令无效,跳转到下一个指令*/
					while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				/*获得子类型*/
				if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//判断是否为type
				{
					subtype = tcp_client_recvbuf[i];
				}
				else
				{
					/*当前指令无效,跳转到下一个指令*/
					while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i+=2;
				if(type == COMMAND_RESULT)//为结果指令
				{
					if(subtype == RES_LOGIN)
					{
						/*判断指令合法性，并且检测是否到一个指令的末尾END*/
						if((tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)&&(tcp_client_recvbuf[i+2] == COMMAND_END))
						{
							res = tcp_client_recvbuf[i];
							if(res == LOGIN_SUCCESS)
							{
								isAuthed = 1;
								LCD_ShowString(30,270,lcddev.width-30,lcddev.height-230,16,"LOGIN SUCCESS");//显示接收到的数据	
							}
							else
							{
								isAuthed = 0;
							}
							i+=3;
							continue;//循环处理下一个指令
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
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
						/*获得开启还是关闭IHome Mode*/
						if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = tcp_client_recvbuf[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
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
						/*-------------返回IHome mode状态信息给用户-----------*/
						send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
						sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
																		RES_IHome,COMMAND_SEPERATOR,
																		ihome_start_flag,COMMAND_SEPERATOR,COMMAND_END);
						if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}
						tcp_client_flag|=1<<7;//标记要发送数据
					}//end of ctl_ihome
					else if(subtype == CTL_GET)
					{
						/*获得哪种设备的ID*/
						if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = tcp_client_recvbuf[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
							while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*获得设备ID*/
						for(j = 0; (tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE)&&(tcp_client_recvbuf[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*暂时用account存返灯ID*/
							account[j] = tcp_client_recvbuf[i];
						}
						i++;
						account[j] = '\0';
						//printf("device ID:%s\n", account);
						if(res == RES_TEMP)
						{
							/*--------------发送TEMP给DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 5);//外部内存分配空间
							sprintf((char *)msg_buf, "TEMP");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
							/*--------------发送温度传感器ID给DHT11任务-----------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//外部内存分配空间
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
						}//end of temp
						else if(res == RES_HUMI)
						{
							/*--------------发送HUMI给DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 5);//外部内存分配空间
							sprintf((char *)msg_buf, "HUMI");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
							/*---------------发送湿度传感器ID给DHT11任务------------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//外部内存分配空间
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}
						}
						
					}//end of ctl_get
					else if(subtype == CTL_LAMP)
					{
						/*获得灯开还是关*/
						if(tcp_client_recvbuf[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = tcp_client_recvbuf[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
							while((tcp_client_recvbuf[i] != '\0') && (tcp_client_recvbuf[i] != COMMAND_END)&&(i<TCP_CLIENT_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*获得灯ID*/
						for(j = 0; (tcp_client_recvbuf[i]!='\0')&&(i<TCP_CLIENT_RX_BUFSIZE)&&(tcp_client_recvbuf[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*暂时用account存返灯ID*/
							account[j] = tcp_client_recvbuf[i];
						}
						i++;
						account[j] = '\0';
						DEBUG("ID：%s \n", account);
						/*发送ON OFF给LED TSAK */
						msg_buf = mymalloc(SRAMEX, 10);//外部内存分配空间
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
						/*发送灯ID给LED任务*/
						msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//外部内存分配空间
						sprintf((char *)msg_buf, (char *)account);
						if(OSQPost(led_event,msg_buf) != OS_ERR_NONE)
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}
					}//end of ctl_lamp
				
				}//end of command contrl
			}
			
			LCD_ShowString(30,250,lcddev.width-30,lcddev.height-230,16,tcp_client_recvbuf);//显示接收到的数据
			
			tcp_client_flag&=~(1<<6);//标记数据已经被处理了.
		}
	  //lwip_periodic_handle();//LWIP轮询任务
  }// end of while(1)
}

//lwIP TCP连接建立后调用回调函数
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
	struct tcp_client_struct *es=NULL;  
	if(err==ERR_OK)   
	{
		es=(struct tcp_client_struct*)mem_malloc(sizeof(struct tcp_client_struct));  //申请内存
		if(es) //内存申请成功
		{
 			es->state=ES_TCPCLIENT_CONNECTED;//状态为连接成功
			es->pcb=tpcb;  
			es->p=NULL; 
			tcp_arg(tpcb,es);        			    //使用es更新tpcb的callback_arg
			tcp_recv(tpcb,tcp_client_recv);  	//初始化LwIP的tcp_recv回调功能   
			tcp_err(tpcb,tcp_client_error); 	//初始化tcp_err()回调函数
			tcp_sent(tpcb,tcp_client_sent);		//初始化LwIP的tcp_sent回调功能
			tcp_poll(tpcb,tcp_client_poll,1); //初始化LwIP的tcp_poll回调功能 
 			tcp_client_flag|=1<<5; 				    //标记连接到服务器了
			err=ERR_OK;
		}else
		{ 
			tcp_client_connection_close(tpcb,es);//关闭连接
			err=ERR_MEM;	//返回内存分配错误
		}
	}else
	{
		tcp_client_connection_close(tpcb,0);//关闭连接
	}
	return err;
}
    
//lwIP tcp_recv()函数的回调函数
err_t tcp_client_recv(void *arg,struct tcp_pcb *tpcb,struct pbuf *p,err_t err)
{ 
	u32 data_len = 0;
	struct pbuf *q;
	struct tcp_client_struct *es;
	err_t ret_err; 
	LWIP_ASSERT("arg != NULL",arg != NULL);
	es=(struct tcp_client_struct *)arg; 
	if(p==NULL)//如果从服务器接收到空的数据帧就关闭连接
	{
		es->state=ES_TCPCLIENT_CLOSING;//需要关闭TCP 连接了 
 		es->p=p; 
		ret_err=ERR_OK;
	}else if(err!= ERR_OK)//当接收到一个非空的数据帧,但是err!=ERR_OK
	{ 
		if(p)pbuf_free(p);//释放接收pbuf
		ret_err=err;
	}else if(es->state==ES_TCPCLIENT_CONNECTED)	//当处于连接状态时
	{
		if(p!=NULL)//当处于连接状态并且接收到的数据不为空时
		{
			memset(tcp_client_recvbuf,0,TCP_CLIENT_RX_BUFSIZE);  //数据接收缓冲区清零
			for(q=p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
			{
				//判断要拷贝到TCP_CLIENT_RX_BUFSIZE中的数据是否大于TCP_CLIENT_RX_BUFSIZE的剩余空间，如果大于
				//的话就只拷贝TCP_CLIENT_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
				if(q->len > (TCP_CLIENT_RX_BUFSIZE-data_len)) memcpy(tcp_client_recvbuf+data_len,q->payload,(TCP_CLIENT_RX_BUFSIZE-data_len));//拷贝数据
				else memcpy(tcp_client_recvbuf+data_len,q->payload,q->len);
				data_len += q->len;  	
				if(data_len > TCP_CLIENT_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
			}
			tcp_client_flag|=1<<6;		//标记接收到数据了
 			tcp_recved(tpcb,p->tot_len);//用于获取接收数据,通知LWIP可以获取更多数据
			pbuf_free(p);  	//释放内存
			ret_err=ERR_OK;
		}
	}else  //接收到数据但是连接已经关闭,
	{ 
		tcp_recved(tpcb,p->tot_len);//用于获取接收数据,通知LWIP可以获取更多数据
		es->p=NULL;
		pbuf_free(p); //释放内存
		ret_err=ERR_OK;
	}
	return ret_err;
}
//lwIP tcp_err函数的回调函数
void tcp_client_error(void *arg,err_t err)
{  
	//这里我们不做任何处理
} 

//lwIP tcp_poll的回调函数
err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb)
{
	u8 *msg;//接收到的消息
	INT8U err;
	
	err_t ret_err;
	struct tcp_client_struct *es; 
	es=(struct tcp_client_struct*)arg;
	if(es!=NULL)  //连接处于空闲可以发送数据
	{
		if(tcp_client_flag&(1<<7))	//判断是否有数据要发送 
		{
			msg = (u8 *)OSQPend (msg_event,  //等待新消息
                0,  //wait forever
                &err);
			if(err != OS_ERR_NONE)
			{
				DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
			}
			es->p=pbuf_alloc(PBUF_TRANSPORT, strlen((char*)msg),PBUF_POOL);	//申请内存 
			pbuf_take(es->p,(char*)msg,strlen((char*)msg));	//将tcp_client_sentbuf[]中的数据拷贝到es->p_tx中
			tcp_client_senddata(tpcb,es);//将tcp_client_sentbuf[]里面复制给pbuf的数据发送出去
			
			myfree(SRAMEX, msg); //清除空间
			tcp_client_flag&=~(1<<7);	//清除数据发送标志
			if(es->p)pbuf_free(es->p);	//释放内存
		}else if(es->state==ES_TCPCLIENT_CLOSING)
		{ 
 			tcp_client_connection_close(tpcb,es);//关闭TCP连接
		} 
		ret_err=ERR_OK;
	}else
	{ 
		tcp_abort(tpcb);//终止连接,删除pcb控制块
		ret_err=ERR_ABRT;
	}
	return ret_err;
} 
//lwIP tcp_sent的回调函数(当从远端主机接收到ACK信号后发送数据)
err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct tcp_client_struct *es;
	LWIP_UNUSED_ARG(len);
	es=(struct tcp_client_struct*)arg;
	if(es->p)tcp_client_senddata(tpcb,es);//发送数据
	return ERR_OK;
}
//此函数用来发送数据
void tcp_client_senddata(struct tcp_pcb *tpcb, struct tcp_client_struct * es)
{
	struct pbuf *ptr; 
 	err_t wr_err=ERR_OK;
	while((wr_err==ERR_OK)&&es->p&&(es->p->len<=tcp_sndbuf(tpcb)))
	{
		ptr=es->p;
		wr_err=tcp_write(tpcb,ptr->payload,ptr->len,1); //将要发送的数据加入到发送缓冲队列中
		if(wr_err==ERR_OK)
		{  
			es->p=ptr->next;			//指向下一个pbuf
			if(es->p)pbuf_ref(es->p);	//pbuf的ref加一
			pbuf_free(ptr);				//释放ptr 
		}else if(wr_err==ERR_MEM)es->p=ptr;
		tcp_output(tpcb);		//将发送缓冲队列中的数据立即发送出去
	} 	
} 
//关闭与服务器的连接
void tcp_client_connection_close(struct tcp_pcb *tpcb, struct tcp_client_struct * es)
{
	//移除回调
	tcp_abort(tpcb);//终止连接,删除pcb控制块
	tcp_arg(tpcb,NULL);  
	tcp_recv(tpcb,NULL);
	tcp_sent(tpcb,NULL);
	tcp_err(tpcb,NULL);
	tcp_poll(tpcb,NULL,0);  
	if(es)mem_free(es); 
	tcp_client_flag&=~(1<<5);//标记连接断开了
}
