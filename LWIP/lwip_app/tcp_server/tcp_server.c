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

//TCP�ͻ�������
#define TCPSERVER_PRIO		6
//�����ջ��С
#define TCPSERVER_STK_SIZE	300
//�����ջ
OS_STK TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE];

//tcp����������
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

	conn = netconn_new(NETCONN_TCP);  //����һ��TCP����
	netconn_bind(conn,IP_ADDR_ANY,TCP_SERVER_PORT);  //�󶨶˿� 8080�Ŷ˿�
	netconn_listen(conn);  		//�������ģʽ
	while (1) 
	{
		err = netconn_accept(conn,&serverconn);  //������������

		if (err == ERR_OK)    //���������ӵ�����
		{ 
			struct netbuf *recvbuf;

			netconn_getaddr(serverconn,&ipaddr,&port,0); //��ȡԶ��IP��ַ�Ͷ˿ں�
			
			remot_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
			remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
			remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
			remot_addr[0] = (uint8_t)(ipaddr.addr);
			printf("����%d.%d.%d.%d�����Ϸ�����,�����˿ں�Ϊ:%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);
			
			isAccepted = 1; //�Ѿ���������
			OSTimeDlyHMSM(0,0,3,0);//�ȴ������֤
			if(isChecked == 0)//���ȷ��ʧ��
			{
				continue;
			}
			while(isAccepted)//��������
			{
				OSTimeDlyHMSM(0,0,2,0);
			}
		}
	}
}

//tcp�ͻ��˷�����Ϣ������
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
			OSTimeDlyHMSM(0,0,2,0);//�ȴ����Ӻ�
		}
		if((recv_err = netconn_recv(serverconn,&recvbuf)) == ERR_OK)  //���յ�����
		{	
			OS_ENTER_CRITICAL(); //���ж�
			recv_msg = mymalloc(SRAMEX, TCP_SERVER_RX_BUFSIZE);//����յ�����Ϣ
			for(q=recvbuf->p;q!=NULL;q=q->next)  //����������pbuf����
			{
					//�ж�Ҫ������TCP_CLIENT_RX_BUFSIZE�е������Ƿ����TCP_CLIENT_RX_BUFSIZE��ʣ��ռ䣬�������
					//�Ļ���ֻ����TCP_CLIENT_RX_BUFSIZE��ʣ�೤�ȵ����ݣ�����Ļ��Ϳ������е�����
					if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len))
					{
						memcpy(recv_msg+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//��������
					}
					else 
					{
						memcpy(recv_msg+data_len,q->payload,q->len);
					}
					data_len += q->len;  	
					if(data_len > TCP_SERVER_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
			}
			OS_EXIT_CRITICAL();  //���ж�
			data_len=0;  				//������ɺ�data_lenҪ���㡣					
			netbuf_delete(recvbuf);
			/*��־�Ǳ�������������Ϣ*/
			if(OSQPost(client_handle_event,"server") != OS_ERR_NONE) 
			{
					DEBUG("server_recv��OSQPost ERROR %d\r\n", __LINE__);
	    }
			/*�����յ�����Ϣ,ת������������*/
			if(OSQPost(client_handle_event,recv_msg) != OS_ERR_NONE) 
			{
					DEBUG("server_recv��OSQPost ERROR %d\r\n", __LINE__);
	    }
		}
		else if(recv_err == ERR_CLSD)  //�ر�����
		{
				netconn_close(serverconn);
				netconn_delete(serverconn);
				DEBUG("������%d.%d.%d.%d�Ͽ�����\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
				isAccepted = 0; //�Ͽ�����
				isChecked = 0;    //�����֤ʧЧ
				continue;
		}//end of netconn_recv
		
	}
}


//����TCP�������߳�
//����ֵ:0 TCP�����������ɹ�
//		���� TCP����������ʧ��
INT8U tcp_server_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//���ж�
	res = OSTaskCreate(tcp_server_thread,(void*)0,(OS_STK*)&TCPSERVER_TASK_STK[TCPSERVER_STK_SIZE-1],TCPSERVER_PRIO); //����TCP�������߳�
	OS_EXIT_CRITICAL();		//���ж�
	
	return res;
}


