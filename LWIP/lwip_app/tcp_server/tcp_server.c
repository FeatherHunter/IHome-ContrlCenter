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

struct netconn *conn;
struct netconn *serverconn;

u8 isAccepted = 0;
u8 isChecked = 0;

//TCP客户端任务
#define TCPSERVER_PRIO		6
//任务堆栈大小
#define TCPSERVER_STK_SIZE	300
//任务堆栈
OS_STK TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE];

//tcp服务器任务
static void tcp_server_thread(void *arg)
{
	OS_CPU_SR cpu_sr;
	u32 data_len = 0;
	struct pbuf *q;
	err_t err,recv_err;
	u8 remot_addr[4];
	static ip_addr_t ipaddr;
	static u16_t 			port;
	
	LWIP_UNUSED_ARG(arg);

	conn = netconn_new(NETCONN_TCP);  //创建一个TCP链接
	netconn_bind(conn,IP_ADDR_ANY,TCP_SERVER_PORT);  //绑定端口 8080号端口
	netconn_listen(conn);  		//进入监听模式
	while (1) 
	{
		err = netconn_accept(conn,&serverconn);  //接收连接请求

		if (err == ERR_OK)    //处理新连接的数据
		{ 
			struct netbuf *recvbuf;

			netconn_getaddr(serverconn,&ipaddr,&port,0); //获取远端IP地址和端口号
			
			remot_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
			remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
			remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
			remot_addr[0] = (uint8_t)(ipaddr.addr);
			printf("主机%d.%d.%d.%d连接上服务器,主机端口号为:%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);
			
			isAccepted = 1; //已经有链接上
			OSTimeDlyHMSM(0,0,3,0);//等待身份验证
			if(isChecked == 0)//身份确认失败
			{
				continue;
			}
			while(isAccepted)//保持链接
			{
				OSTimeDlyHMSM(0,0,2,0);
			}
		}
	}
}

//tcp客户端发送信息任务函数
void tcp_server_recv(void *arg)
{
	u8 * recv_msg;
	err_t recv_err;
	u32 data_len = 0;
	OS_CPU_SR cpu_sr;
	struct netbuf *recvbuf;
	struct pbuf *q;
	DEBUG("tcp server recv task\r\n");
	while(1)
	{
		while(!isAccepted)
		{
			OSTimeDlyHMSM(0,0,2,0);//等待连接好
		}
		if((recv_err = netconn_recv(serverconn,&recvbuf)) == ERR_OK)  //接收到数据
		{	
			OS_ENTER_CRITICAL(); //关中断
			recv_msg = mymalloc(SRAMEX, TCP_SERVER_RX_BUFSIZE);//存放收到的信息
			for(q=recvbuf->p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
			{
					//判断要拷贝到TCP_CLIENT_RX_BUFSIZE中的数据是否大于TCP_CLIENT_RX_BUFSIZE的剩余空间，如果大于
					//的话就只拷贝TCP_CLIENT_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
					if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len))
					{
						memcpy(recv_msg+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//拷贝数据
					}
					else 
					{
						memcpy(recv_msg+data_len,q->payload,q->len);
					}
					data_len += q->len;  	
					if(data_len > TCP_SERVER_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
			}
			OS_EXIT_CRITICAL();  //开中断
			data_len=0;  				//复制完成后data_len要清零。					
			netbuf_delete(recvbuf);
			/*标志是本机服务器端信息*/
			if(OSQPost(client_handle_event,"server") != OS_ERR_NONE) 
			{
					DEBUG("server_recv：OSQPost ERROR %d\r\n", __LINE__);
	    }
			/*发送收到的信息,转发给处理任务*/
			if(OSQPost(client_handle_event,recv_msg) != OS_ERR_NONE) 
			{
					DEBUG("server_recv：OSQPost ERROR %d\r\n", __LINE__);
	    }
		}
		else if(recv_err == ERR_CLSD)  //关闭连接
		{
				netconn_close(serverconn);
				netconn_delete(serverconn);
				DEBUG("服务器%d.%d.%d.%d断开连接\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
				isAccepted = 0; //断开连接
				isChecked = 0;    //身份认证失效
				continue;
		}//end of netconn_recv
		
	}
}


//创建TCP服务器线程
//返回值:0 TCP服务器创建成功
//		其他 TCP服务器创建失败
INT8U tcp_server_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//关中断
	res = OSTaskCreate(tcp_server_thread,(void*)0,(OS_STK*)&TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE-1],TCPSERVER_PRIO); //创建TCP服务器线程
	OS_EXIT_CRITICAL();		//开中断
	
	return res;
}


