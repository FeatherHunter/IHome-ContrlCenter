#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H
#include "sys.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/memp.h"
#include "lwip/mem.h"
#include "lwip_comm.h"
#include "task_priority.h"
#include "ihome_function.h"   
#include "malloc.h"
#include "message_queue.h"

#define ACCOUNT_MAX 32
#define COMMAND_PULSE  '0'
#define COMMAND_MANAGE 1
#define COMMAND_CONTRL 2
#define COMMAND_RESULT 3
#define MAN_LOGIN 11
#define CTL_LAMP  21
#define CTL_GET   22
#define RES_LOGIN 32
#define RES_LAMP  33
#define RES_TEMP  34
#define RES_HUMI  35
#define LOGIN_SUCCESS 1
#define LOGIN_FAILED  2
#define LAMP_ON  1
#define LAMP_OFF 2
 
#define COMMAND_END 30
#define COMMAND_SEPERATOR 31

#define MASTER "975559549"
#define SLAVE  "975559549h"

extern u8 isConnected;
extern u8 isAuthed;

#define TCP_CLIENT_RX_BUFSIZE	100	//����tcp client���������ݳ���
#define TCP_CLIENT_TX_BUFSIZE	2000		//����tcp client��������ݳ���
#define REMOTE_PORT				8080	//����Զ��������IP��ַ
#define LWIP_SEND_DATA			0X80    //���������ݷ���
#define	TCP_CLIENT_PORT			8080	//����tcp clientҪ���ӵ�Զ�˶˿�

/*TCP Client�������ݻ����������ͻ�����*/
extern u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	
extern u8 *tcp_client_sendbuf;
extern u8 tcp_client_flag;

//tcp����������״̬
enum tcp_client_states
{
	ES_TCPCLIENT_NONE = 0,		//û������
	ES_TCPCLIENT_CONNECTED,		//���ӵ��������� 
	ES_TCPCLIENT_CLOSING,		//�ر�����
}; 
//LWIP�ص�����ʹ�õĽṹ��
struct tcp_client_struct
{
	u8 state;               //��ǰ����״
	struct tcp_pcb *pcb;    //ָ��ǰ��pcb
	struct pbuf *p;         //ָ�����/�����pbuf
};  

void tcp_client_set_remoteip(void);
err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
err_t tcp_client_recv(void *arg,struct tcp_pcb *tpcb,struct pbuf *p,err_t err);
void tcp_client_error(void *arg,err_t err);
err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb);
err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
void tcp_client_senddata(struct tcp_pcb *tpcb, struct tcp_client_struct * es);
void tcp_client_connection_close(struct tcp_pcb *tpcb, struct tcp_client_struct * es );
#endif
