/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2016.1.9
 * @filename task_server.c
 * @description tcp server相关功能，包括等待用户连接，接收数据和处理数据。
 * @FunctionList
 *		1.INT8U tcp_server_init(void); //create tasks
 *		2.void tcp_server_task(void *arg); //waiting user's connection
 *    3.void tcp_server_recv(void *arg); //receive user's messages
 */ 
#include "tcp_server.h"
#include "malloc.h"
#include "idebug.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include "led.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"
#include "tcp_client.h"
#include "message_queue.h"
#include "main.h"

struct netconn *conn;
struct netconn *serverconn;

struct netconn *videoconn;
struct netconn *video_server;

u8 isAccepted = 0;
u8 isChecked  = 0;

/*TCP server task stk*/
OS_STK * TCPSERVER_TASK_STK;
/*TCP server recv stk*/
OS_STK * TCPSERVER_RECV_TASK_STK;	

u8 user_addr[4];
u8 video_addr[4];

/**
 * @Function: INT8U tcp_server_init(void)
 * @Description: create tasks for server
 * @Return 0: TCP server create success
 *				 other: TCP server create failed
 */
INT8U tcp_server_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	TCPSERVER_RECV_TASK_STK    = mymalloc(SRAMEX, TCPSERVER_RECV_STK_SIZE*sizeof(OS_STK));
	TCPSERVER_TASK_STK         = mymalloc(SRAMEX, TCPSERVER_STK_SIZE*sizeof(OS_STK));
	
	OS_ENTER_CRITICAL();	//关中断
	res = OSTaskCreate(tcp_server_task,(void*)0 ,(OS_STK*)&TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE-1], TCPSERVER_PRIO); //创建TCP服务器线程
	res += OSTaskCreate(tcp_server_recv,(void*)0,(OS_STK*)&TCPSERVER_RECV_TASK_STK[TCPSERVER_RECV_STK_SIZE-1], TCPSERVER_RECV_TASK_PRIO);
	OS_EXIT_CRITICAL();		//开中断
	
	return res;
}

/**
 * @Function: void tcp_server_thread(void *arg)
 * @Description: stm32 wait user's connection as a server
 *       isAccepted: a flag of accepting a user
 *       isChecked : a flag of checking user's id
 */
void tcp_server_task(void *arg)
{
	err_t err;
	u8 * msg_buf;
	static ip_addr_t ipaddr;
	static u16_t 			port;
	
	LWIP_UNUSED_ARG(arg);
	conn = netconn_new(NETCONN_TCP);  //创建一个TCP链接
	err = netconn_bind(conn,IP_ADDR_ANY,TCP_SERVER_PORT);  //绑定端口 8080号端口
	err = netconn_listen(conn);  		//进入监听模式
	while (1) 
	{
		printf("accepting!\n");
		DEBUG_LCD(20, 160, "Accepting......      ", RED);//显示接收到的数据	
		err = netconn_accept(conn,&serverconn);  //接收连接请求
		printf("accepted!\n");
		if (err == ERR_OK)    //处理新连接的数据
		{ 
			netconn_getaddr(serverconn,&ipaddr,&port,0); //获取远端IP地址和端口号
			user_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
			user_addr[2] = (uint8_t)(ipaddr.addr>> 16);
			user_addr[1] = (uint8_t)(ipaddr.addr >> 8);
			user_addr[0] = (uint8_t)(ipaddr.addr);
			printf("主机%d.%d.%d.%d连接上服务器,主机端口号为:%d\r\n",user_addr[0], user_addr[1],user_addr[2],user_addr[3],port);
			
			isAccepted = 1; //已经有链接上
			OSTimeDlyHMSM(0,0,3,0);//等待身份验证
			while(isAccepted)//保持链接
			{
				if(isChecked == 0)//身份确认失败
				{
					DEBUG_LCD(20, 160, "Client Checking...      ", BLUE);//显示接收到的数据	
					DEBUG("stm32 is cheking user's id\r\n");
					OSTimeDlyHMSM(0,0,2,0);
				}
				else
				{
					DEBUG("user is checked\r\n");
//					/*--------------发送任意信息给用户,确认连接状态------------------------------*/
//					msg_buf = mymalloc(SRAMEX, 7);
//					sprintf((char *)msg_buf, "server");
//					if(OSQPost(tcp_send_event, msg_buf) != OS_ERR_NONE)//为客户端信息
//					{
//							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
//					}	
//					msg_buf = mymalloc(SRAMEX, 4);//外部内存分配空间
//					sprintf((char *)msg_buf, "%c%c%c",
//																		COMMAND_PULSE,COMMAND_SEPERATOR,COMMAND_END);
//					if(OSQPost(tcp_send_event,msg_buf) != OS_ERR_NONE)
//					{
//							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
//					}
					OSTimeDlyHMSM(0,0,10,0);
				}
				OSTimeDlyHMSM(0,0,2,0);
			}
		}
		else
		{
			DEBUG("tcp_server_task accept error!\r\n");
		}
	}//end of while(1)
}

/**
 * @Function: void tcp_server_recv(void *arg)
 * @Description: 本机作为服务器，接收来自用户的控制信息、
 *			 接收到消息后，将其转发给信息处理任务，并注明是本机作为服务器的信息
 *       isAccepted: 有用户链接上的标志位
 */
void tcp_server_recv(void *arg)
{
	u8 * auth_msg;
	u8 * recv_msg;
	err_t recv_err;
	u32 data_len = 0;
	OS_CPU_SR cpu_sr;
	struct netbuf *recvbuf;
	struct pbuf *q;
	DEBUG("Server recv task\r\n");
	while(1)
	{
		/*等待用户连接成功*/
		while(!isAccepted)
		{
			OSTimeDlyHMSM(0,0,2,0);
		}
		/*接收数据*/
		if((recv_err = netconn_recv(serverconn,&recvbuf)) == ERR_OK)  //接收到数据
		{	
			OS_ENTER_CRITICAL(); //关中断
			recv_msg = mymalloc(SRAMEX, TCP_RX_BUFSIZE);//存放收到的信息
			for(q=recvbuf->p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
			{
					if(q->len > (TCP_RX_BUFSIZE-data_len))
					{
						memcpy(recv_msg+data_len,q->payload,(TCP_RX_BUFSIZE-data_len));//拷贝数据
					}
					else 
					{
						memcpy(recv_msg+data_len,q->payload,q->len);
					}
					data_len += q->len;  	
					if(data_len > TCP_RX_BUFSIZE) break; //超出TCP服务器能接受的大小,跳出	
			}
			OS_EXIT_CRITICAL();  //开中断
			data_len=0;  				//复制完成后data_len要清零。					
			netbuf_delete(recvbuf);
			/*发送收到的信息,转发给处理任务*/
			auth_msg = mymalloc(SRAMEX, 7);
			sprintf((char *)auth_msg, "server");
			if(OSQPost(tcp_handle_event, auth_msg) != OS_ERR_NONE)//为客户端信息
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
			}		
			if(OSQPost(tcp_handle_event, recv_msg) != OS_ERR_NONE) 
			{
					DEBUG("server_recv：OSQPost ERROR %d\r\n", __LINE__);
	    }
		}
		else if(recv_err == ERR_CLSD)  //关闭连接
		{
				netconn_close(serverconn);
				netconn_delete(serverconn);
				DEBUG("USER %d.%d.%d.%d断开连接\r\n",user_addr[0],user_addr[1], user_addr[2],user_addr[3]);
				isAccepted = 0; //断开连接
				isChecked = 0;    //身份认证失效
				continue;
		}//end of netconn_recv
		
	}
}


