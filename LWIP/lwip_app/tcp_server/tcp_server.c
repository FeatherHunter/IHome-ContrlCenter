/**
 * @copyright ������(2014~2024) QQ��975559549
 * @Author Feather @version 1.0 @date 2016.1.9
 * @filename task_server.c
 * @description tcp server��ع��ܣ������ȴ��û����ӣ��������ݺʹ������ݡ�
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
	
	OS_ENTER_CRITICAL();	//���ж�
	res = OSTaskCreate(tcp_server_task,(void*)0 ,(OS_STK*)&TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE-1], TCPSERVER_PRIO); //����TCP�������߳�
	res += OSTaskCreate(tcp_server_recv,(void*)0,(OS_STK*)&TCPSERVER_RECV_TASK_STK[TCPSERVER_RECV_STK_SIZE-1], TCPSERVER_RECV_TASK_PRIO);
	OS_EXIT_CRITICAL();		//���ж�
	
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
	conn = netconn_new(NETCONN_TCP);  //����һ��TCP����
	err = netconn_bind(conn,IP_ADDR_ANY,TCP_SERVER_PORT);  //�󶨶˿� 8080�Ŷ˿�
	err = netconn_listen(conn);  		//�������ģʽ
	while (1) 
	{
		printf("accepting!\n");
		DEBUG_LCD(20, 160, "Accepting......      ", RED);//��ʾ���յ�������	
		err = netconn_accept(conn,&serverconn);  //������������
		printf("accepted!\n");
		if (err == ERR_OK)    //���������ӵ�����
		{ 
			netconn_getaddr(serverconn,&ipaddr,&port,0); //��ȡԶ��IP��ַ�Ͷ˿ں�
			user_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
			user_addr[2] = (uint8_t)(ipaddr.addr>> 16);
			user_addr[1] = (uint8_t)(ipaddr.addr >> 8);
			user_addr[0] = (uint8_t)(ipaddr.addr);
			printf("����%d.%d.%d.%d�����Ϸ�����,�����˿ں�Ϊ:%d\r\n",user_addr[0], user_addr[1],user_addr[2],user_addr[3],port);
			
			isAccepted = 1; //�Ѿ���������
			OSTimeDlyHMSM(0,0,3,0);//�ȴ������֤
			while(isAccepted)//��������
			{
				if(isChecked == 0)//���ȷ��ʧ��
				{
					DEBUG_LCD(20, 160, "Client Checking...      ", BLUE);//��ʾ���յ�������	
					DEBUG("stm32 is cheking user's id\r\n");
					OSTimeDlyHMSM(0,0,2,0);
				}
				else
				{
					DEBUG("user is checked\r\n");
//					/*--------------����������Ϣ���û�,ȷ������״̬------------------------------*/
//					msg_buf = mymalloc(SRAMEX, 7);
//					sprintf((char *)msg_buf, "server");
//					if(OSQPost(tcp_send_event, msg_buf) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
//					{
//							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
//					}	
//					msg_buf = mymalloc(SRAMEX, 4);//�ⲿ�ڴ����ռ�
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
 * @Description: ������Ϊ�����������������û��Ŀ�����Ϣ��
 *			 ���յ���Ϣ�󣬽���ת������Ϣ�������񣬲�ע���Ǳ�����Ϊ����������Ϣ
 *       isAccepted: ���û������ϵı�־λ
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
		/*�ȴ��û����ӳɹ�*/
		while(!isAccepted)
		{
			OSTimeDlyHMSM(0,0,2,0);
		}
		/*��������*/
		if((recv_err = netconn_recv(serverconn,&recvbuf)) == ERR_OK)  //���յ�����
		{	
			OS_ENTER_CRITICAL(); //���ж�
			recv_msg = mymalloc(SRAMEX, TCP_RX_BUFSIZE);//����յ�����Ϣ
			for(q=recvbuf->p;q!=NULL;q=q->next)  //����������pbuf����
			{
					if(q->len > (TCP_RX_BUFSIZE-data_len))
					{
						memcpy(recv_msg+data_len,q->payload,(TCP_RX_BUFSIZE-data_len));//��������
					}
					else 
					{
						memcpy(recv_msg+data_len,q->payload,q->len);
					}
					data_len += q->len;  	
					if(data_len > TCP_RX_BUFSIZE) break; //����TCP�������ܽ��ܵĴ�С,����	
			}
			OS_EXIT_CRITICAL();  //���ж�
			data_len=0;  				//������ɺ�data_lenҪ���㡣					
			netbuf_delete(recvbuf);
			/*�����յ�����Ϣ,ת������������*/
			auth_msg = mymalloc(SRAMEX, 7);
			sprintf((char *)auth_msg, "server");
			if(OSQPost(tcp_handle_event, auth_msg) != OS_ERR_NONE)//Ϊ�ͻ�����Ϣ
			{
					DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
			}		
			if(OSQPost(tcp_handle_event, recv_msg) != OS_ERR_NONE) 
			{
					DEBUG("server_recv��OSQPost ERROR %d\r\n", __LINE__);
	    }
		}
		else if(recv_err == ERR_CLSD)  //�ر�����
		{
				netconn_close(serverconn);
				netconn_delete(serverconn);
				DEBUG("USER %d.%d.%d.%d�Ͽ�����\r\n",user_addr[0],user_addr[1], user_addr[2],user_addr[3]);
				isAccepted = 0; //�Ͽ�����
				isChecked = 0;    //�����֤ʧЧ
				continue;
		}//end of netconn_recv
		
	}
}


