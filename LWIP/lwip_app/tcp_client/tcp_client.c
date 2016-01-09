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

struct netconn *tcp_clientconn;					//TCP CLIENT网络连接结构体
u8 tcp_client_flag;		//TCP客户端数据发送标志位

u8 isConnected = 0;
u8 isAuthed    = 0;

/*TCP 客户端堆栈*/
OS_STK * TCP_CLIENT_CONNECT_TASK_STK;	//链接
OS_STK * TCP_CLIENT_RECV_TASK_STK;	  //接收
OS_STK * CLIENT_HANDLE_TASK_STK;	    //处理
/*TCP send stack*/
OS_STK * TCP_SEND_TASK_STK;	          //发送

/*handle消息队列*/
void *tcp_handle_q[HANDLESIZE];
OS_EVENT * tcp_handle_event;
/*send消息队列*/
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
	
	OS_ENTER_CRITICAL();	//关中断
	
	res = OSTaskCreate(tcp_client_connect ,(void*)0, (OS_STK*)&TCP_CLIENT_CONNECT_TASK_STK[TCP_CLIENT_CONNECT_STK_SIZE-1], TCP_CLIENT_CONNECT_TASK_PRIO); //创建TCP客户端连接任务
	res += OSTaskCreate(tcp_client_recv   ,(void*)0, (OS_STK*)&TCP_CLIENT_RECV_TASK_STK[TCP_CLIENT_RECV_STK_SIZE-1]      , TCP_CLIENT_RECV_TASK_PRIO);    //创建TCP客户端接受任务
	res += OSTaskCreate(tcp_handle_task,(void*)0, (OS_STK*)&CLIENT_HANDLE_TASK_STK[CLIENT_HANDLE_STK_SIZE-1]          , CLIENT_HANDLE_TASK_PRIO);      //创建TCP客户端处理任务
	res += OSTaskCreate(tcp_send_task   ,(void*)0, (OS_STK*)&TCP_SEND_TASK_STK[TCP_SEND_STK_SIZE-1]      , TCP_SEND_TASK_PRIO);    //创建TCP客户端发送任务
	
	OS_EXIT_CRITICAL();		//开中断
	
	return res;
}

//tcp客户端任务函数
void tcp_client_connect(void *arg)
{
	u8 * send_buf; //发送消息的缓冲
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
			OSTimeDlyHMSM(0,0,2,0);//还在连接中睡眠2s
		}
		/*--------重新连接---------*/
		if(isConnected == 0)
		{
			tcp_clientconn = netconn_new(NETCONN_TCP);  //创建一个TCP链接
			err = netconn_connect(tcp_clientconn,&server_ipaddr,server_port);//连接服务器
			if(err != ERR_OK)  netconn_delete(tcp_clientconn); //返回值不等于ERR_OK,删除tcp_clientconn连接
			else
			{
				netconn_getaddr(tcp_clientconn,&loca_ipaddr,&loca_port,1); //获取本地IP主机IP地址和端口号
			  printf("连接上服务器%d.%d.%d.%d,本机端口号为:%d\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3],loca_port);
				isConnected = 1;
			  isAuthed = 0;
			}
			POINT_COLOR = BLUE;
			LCD_ShowString(20,100,200,20,16,"connecting...");//显示接收到的数据	
			DEBUG("is connecting!\n");
		}
		/*--------认证信息---------*/
		if((isConnected != 0)&&(isAuthed == 0))
		{
			/*发送身份指令*/
			send_buf = mymalloc(SRAMEX, 7);
			sprintf((char *)send_buf, "client");
			if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
			}		
			
			send_buf = mymalloc(SRAMEX, 73); //32(account)+32(password)+7+1 认证信息最高上限
			sprintf((char *)send_buf, "%c%c975559549h%c%c%c975559549%c%c",COMMAND_MANAGE,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR,
																			MAN_LOGIN,COMMAND_SEPERATOR,
																			COMMAND_SEPERATOR, COMMAND_END);
			if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE) //认证信息提交给发送任务
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
	    }
			POINT_COLOR = BLUE;
			LCD_ShowString(20,100,200,20,16,"authting.....");//显示接收到的数据	
			DEBUG("is authing!\n");
		}
		OSTimeDlyHMSM(0,0,1,0);//还在连接中睡眠2s
	}
}

//tcp客户端发送信息任务函数
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
			OSTimeDlyHMSM(0,0,2,0);//等待连接好
		}
		if((recv_err = netconn_recv(tcp_clientconn,&recvbuf)) == ERR_OK)  //接收到数据
		{	
			OS_ENTER_CRITICAL(); //关中断
			recv_msg = mymalloc(SRAMEX, TCP_RX_BUFSIZE);//存放收到的信息
			for(q=recvbuf->p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
			{
					//判断要拷贝到TCP_RX_BUFSIZE中的数据是否大于TCP_RX_BUFSIZE的剩余空间，如果大于
					//的话就只拷贝TCP_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
					if(q->len > (TCP_RX_BUFSIZE-data_len))
					{
						memcpy(recv_msg+data_len,q->payload,(TCP_RX_BUFSIZE-data_len));//拷贝数据
					}
					else 
					{
						memcpy(recv_msg+data_len,q->payload,q->len);
					}
					data_len += q->len;  	
					if(data_len > TCP_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
			}
			OS_EXIT_CRITICAL();  //开中断
			data_len=0;  				//复制完成后data_len要清零。					
			netbuf_delete(recvbuf);
			/*发送收到的信息,转发给处理任务*/
			auth_msg = mymalloc(SRAMEX, 7);
			sprintf((char *)auth_msg, "client");
			if(OSQPost(tcp_handle_event, auth_msg) != OS_ERR_NONE)//为客户端信息
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
			}		
			if(OSQPost(tcp_handle_event, recv_msg) != OS_ERR_NONE) 
			{
					DEBUG("tcp_client_recv：OSQPost ERROR %d\r\n", __LINE__);
	    }
		}
		else if(recv_err == ERR_CLSD)  //关闭连接
		{
				netconn_close(tcp_clientconn);
				netconn_delete(tcp_clientconn);
				DEBUG("服务器%d.%d.%d.%d断开连接\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
				isConnected = 0; //断开连接
				isAuthed = 0;    //身份认证失效
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
		auth_msg = (u8 *)OSQPend (tcp_handle_event,  //等待服务器端 or 客户端 信息
                0,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			 DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
		}
		recv_msg = (u8 *)OSQPend (tcp_handle_event,  //等待消息
                1000,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			 DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
		}
		if(strcmp((char *)auth_msg, "client") == 0)//是客户端
		{
			
			i = 0;
			/*解析接收到的信息(可能包含多个指令)*/
			while((recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE))
			{
				if((recv_msg[i]!='\0') && recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否为type
				{
					type = recv_msg[i];
				}
				else
				{
					/*当前指令无效,跳转到下一个指令*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i += 2;
				/*记录账号*/
				for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
				{
					account[j] = recv_msg[i];
				}
				i++;
				account[j] = '\0';
				if((strcmp(account, master) != 0)&&(strcmp(account, slave) != 0)) //排除非SLAVE和MASTER的信息
				{
					/*当前指令无效,跳转到下一个指令*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				/*获得子类型*/
				if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否为type
				{
					subtype = recv_msg[i];
				}
				else
				{
					/*当前指令无效,跳转到下一个指令*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
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
						if((recv_msg[i+1] == COMMAND_SEPERATOR)&&(recv_msg[i+2] == COMMAND_END))
						{
							res = recv_msg[i];
							if(res == LOGIN_SUCCESS)
							{
								isAuthed = 1;
								POINT_COLOR = GREEN;
								LCD_ShowString(20,120,200,20,16,"Login Success");//显示接收到的数据	
							}
							else
							{
								POINT_COLOR = GREEN;
								LCD_ShowString(20,120,200,20,16,"Login Failed ");//显示接收到的数据
								isAuthed = 0;
							}
							i+=3;
							continue;//循环处理下一个指令
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
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
						/*获得开启还是关闭IHome Mode*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = recv_msg[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
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
						/*-------------返回IHome mode状态信息给用户-----------*/
						send_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)send_buf, "client");
						if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}				
						send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
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
						/*获得哪种设备的ID*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = recv_msg[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*获得设备ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*暂时用account存返灯ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						//printf("device ID:%s\n", account);
						if(res == RES_TEMP)
						{
							/*--------------发送TEMP给DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "client");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//为客户端信息
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
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
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "client");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//为客户端信息
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							msg_buf = mymalloc(SRAMEX, 5);//外部内存分配空间
							sprintf((char *)msg_buf, "HUMI");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
							/*---------------发送湿度传感器ID给DHT11任务------------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//外部内存分配空间
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
						}
						
					}//end of ctl_get
					else if(subtype == CTL_LAMP)
					{
						/*获得灯开还是关*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = recv_msg[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*获得灯ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*暂时用account存返灯ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						DEBUG("ID：%s \n", account);
						/*表示是收到服务器控制信息*/
						msg_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)msg_buf, "client");
						if(OSQPost(led_event, msg_buf) != OS_ERR_NONE)//为客户端信息
						{
							 DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}	
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
			}//end of while(msg)
			
		}//end of client;
		else if(strcmp((char *)auth_msg, "server") == 0)//处理作为服务器信息
		{
			
			i = 0;
			/*解析接收到的信息(可能包含多个指令)*/
			while((recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE))
			{
				if((recv_msg[i]!='\0') && recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否为type
				{
					type = recv_msg[i];
				}
				else
				{
					/*当前指令无效,跳转到下一个指令*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i += 2;
				/*记录账号*/
				for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
				{
					account[j] = recv_msg[i];
				}
				i++;
				account[j] = '\0';
				if((strcmp(account, master) != 0)&&(strcmp(account, slave) != 0)) //排除非SLAVE和MASTER的信息
				{
					/*当前指令无效,跳转到下一个指令*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				/*获得子类型*/
				if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否为type
				{
					subtype = recv_msg[i];
				}
				else
				{
					/*当前指令无效,跳转到下一个指令*/
					while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
          {
                i++;
          }
          i++;
          continue;
				}
				i+=2;
				if(type == COMMAND_MANAGE)//登录指令
				{
					if(subtype == MAN_LOGIN)
					{
						if(strcmp(account, master) == 0)//接收到用户发来的链接请求
						{
							/*暂时用account记录密码*/
							for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
							{
								account[j] = recv_msg[i];
							}
							i++;
							account[j] = '\0';
							
							/*-------------return msg to user-----------*/
							send_buf = mymalloc(SRAMEX, 7);
						  sprintf((char *)send_buf, "server");
							if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
							{
									DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							if(strcmp(account, password) == 0)//登陆成功
							{
									/*-------------return msg about login_success to user-----------*/		
									send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
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
									send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
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
						/*获得开启还是关闭IHome Mode*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = recv_msg[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
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
						/*-------------返回IHome mode状态信息给用户-----------*/
						send_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)send_buf, "server");
						if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
						{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}				
						send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
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
						/*获得哪种设备的ID*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = recv_msg[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*获得设备ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*暂时用account存返灯ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						//printf("device ID:%s\n", account);
						if(res == RES_TEMP)
						{
							/*--------------发送TEMP给DHT11 TSAK------------------------------*/
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "server");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//为客户端信息
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
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
							msg_buf = mymalloc(SRAMEX, 7);
							sprintf((char *)msg_buf, "server");
							if(OSQPost(dht11_event, msg_buf) != OS_ERR_NONE)//为客户端信息
							{
								DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
							}	
							msg_buf = mymalloc(SRAMEX, 5);//外部内存分配空间
							sprintf((char *)msg_buf, "HUMI");
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
							/*---------------发送湿度传感器ID给DHT11任务------------------------*/
							msg_buf = mymalloc(SRAMEX, ACCOUNT_MAX + 1);//外部内存分配空间
							sprintf((char *)msg_buf, (char *)account);
							if(OSQPost(dht11_event,msg_buf) != OS_ERR_NONE)
							{
								DEBUG("OSQPost ERROR \n file:%s \n line: %d\n", __FILE__, __LINE__);
							}
						}
						
					}//end of ctl_get
					else if(subtype == CTL_LAMP)
					{
						/*获得灯开还是关*/
						if(recv_msg[i+1] == COMMAND_SEPERATOR)//判断是否合法
						{
							res = recv_msg[i];
						}
						else
						{
							/*当前指令无效,跳转到下一个指令*/
							while((recv_msg[i] != '\0') && (recv_msg[i] != COMMAND_END)&&(i<TCP_RX_BUFSIZE))//msg[i]=END
							{
									i++;
							}
							i++;
							continue;
						}
						i+=2;
						/*获得灯ID*/
						for(j = 0; (recv_msg[i]!='\0')&&(i<TCP_RX_BUFSIZE)&&(recv_msg[i]!=COMMAND_SEPERATOR)&&(j <= ACCOUNT_MAX); i++, j++)
						{
							/*暂时用account存返灯ID*/
							account[j] = recv_msg[i];
						}
						i++;
						account[j] = '\0';
						DEBUG("ID：%s \n", account);
						/*表示是收到服务器控制信息*/
						msg_buf = mymalloc(SRAMEX, 7);
						sprintf((char *)msg_buf, "server");
						if(OSQPost(led_event, msg_buf) != OS_ERR_NONE)//为客户端信息
						{
							 DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
						}	
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
	u8 *auth_msg; //身份信息
	u8 *msg;      //接收到的消息
	INT8U err;
	DEBUG("tcp send task\r\n");
	while(1)
	{
		while(!isConnected)
		{
			OSTimeDlyHMSM(0,0,2,0);//等待连接好
		}
		auth_msg = (u8 *)OSQPend (tcp_send_event,  //等待新消息
                0,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("rev auth_msg err! %s %d\n", __FILE__, __LINE__);
			myfree(SRAMEX, auth_msg);
			continue; //下一次循环
		}
		msg = (u8 *)OSQPend (tcp_send_event,  //等待新消息
                2000,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			 DEBUG("rev err! %s %d\n", __FILE__, __LINE__);
		}
		else
		{
			if(strcmp((char *)auth_msg, "client") == 0)//客户端发送给服务器
			{
					err = netconn_write(tcp_clientconn ,msg,strlen((char*)msg),NETCONN_COPY); //发送tcp_server_sentbuf中的数据
					if(err != ERR_OK)
					{
						DEBUG("发送失败\r\n");
						isConnected = 0;
					}
			}
			else if(strcmp((char *)auth_msg, "server") == 0)//作为服务器发送给用户
			{
					err = netconn_write(serverconn ,msg,strlen((char*)msg),NETCONN_COPY); //发送tcp_server_sendbuf中的数据
					if(err != ERR_OK)
					{
						DEBUG("发送失败\r\n");
						isAccepted = 0;
					}
			}
			

			myfree(SRAMEX, msg); //释放消息的空间，防止内存泄露
		}
		myfree(SRAMEX, auth_msg);
	}
	
}

