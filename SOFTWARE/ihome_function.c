#include "main.h"
#include "ihome_function.h"
#include "lsens.h"

/**
 * @copyright 王辰浩(2015~2025) QQ：975559549
 * @Author Feather @version 2.0 @date 2016.1.4
 * @filename ihome_function.c
 * @description IHome相关任务
 * @FunctionList
 *		1.void led_task(void *pdata); //lamp task
 *		2.void dht11_task(void *pdata); //dht11 task about temp and humi
 */ 
 
/*开启智能家居模式的标志
 *START:已经开启
 *STOP:关闭智能家居模式*/
int ihome_start_flag = IHome_STOP;
 
 
/*LED任务堆栈*/
OS_STK	* LED_TASK_STK;
/*DHT11任务堆栈*/
OS_STK	* DHT11_TASK_STK;

/*led消息队列*/
void *led_q[MSGSIZE];
OS_EVENT * led_event;

/*dht11 消息队列*/
void *dht11_q[DHT11SIZE];
OS_EVENT * dht11_event;
 
/**
 * @Function void led_task(void *pdata);
 * @description 处理灯的任务。
 *						收到的消息：ON:开灯 OFF:关灯 设备ID
 * @Input void *pdata 参数
 * @Return NULL
 */
void led_task(void *pdata)
{
	char * auth_msg = NULL;
	char * msg = NULL;
	char * id_msg = NULL;
	u8 * send_buf;
	u32 lsens;
	INT8U err;
	
	while(1)
	{
		DEBUG("led task!\r\n");
		LED1 =! LED1;
		if(ihome_start_flag == IHome_START)//开启了IHome模式
		{
			lsens = (Lsens_Get_Val()+10)*5;
			if(lsens >= 500)
			{
				TIM_SetCompare1(TIM14, 500);
			}
			else
			{
				TIM_SetCompare1(TIM14, lsens);
			}
		}
		auth_msg = (char *)OSQPend (led_event,
                400,  //wait 500ms
                &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
			continue; //没有接收到指令
		}
		if(strcmp(auth_msg, "client") == 0)
		{
			msg = (char *)OSQPend (led_event,
                500,  //wait 500ms
                &err);
			if(err != OS_ERR_NONE)
			{
				DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
				continue; //没有接收到指令
			}
			if(strcmp(msg, "ON") == 0)
			{
				myfree(SRAMEX, msg);
				id_msg = (char *)OSQPend (led_event,
                500,  //wait 2000ms
                &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //没有接收到指令
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 0);
					/*发送身份给发送任务*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}		
					/*发送状态信息*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
																		RES_LAMP,COMMAND_SEPERATOR,
																		LAMP_ON, COMMAND_SEPERATOR, 
																		"0", COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}
			else if(strcmp(msg, "OFF") == 0)
			{
				myfree(SRAMEX, msg);
				id_msg = (char *)OSQPend (led_event,
                 500,  //wait 2000ms
                 &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //没有接收到指令
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 500);
					/*发送身份给发送任务*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					/*发送状态信息*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
 																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		RES_LAMP,COMMAND_SEPERATOR,
 																		LAMP_OFF, COMMAND_SEPERATOR, 
 																		"0", COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}
		
	  }//client
		else if(strcmp(auth_msg, "server") == 0)
		{
			msg = (char *)OSQPend (led_event,
                500,  //wait 500ms
                &err);
			if(err != OS_ERR_NONE)
			{
				DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
				continue; //没有接收到指令
			}
			if(strcmp(msg, "ON") == 0)
			{
				myfree(SRAMEX, msg);
				id_msg = (char *)OSQPend (led_event,
                500,  //wait 2000ms
                &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //没有接收到指令
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 0);
					/*发送身份给发送任务*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}		
					/*发送状态信息*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%s%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,
																		slave,COMMAND_SEPERATOR,
																		RES_LAMP,COMMAND_SEPERATOR,
																		LAMP_ON, COMMAND_SEPERATOR, 
																		"0", COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}
			else if(strcmp(msg, "OFF") == 0)
			{
				myfree(SRAMEX, msg);
				id_msg = (char *)OSQPend (led_event,
                 500,  //wait 2000ms
                 &err);
				if(err != OS_ERR_NONE)
				{	
					DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
					continue; //没有接收到指令
				}
				if(strcmp(id_msg, "0") == 0)
				{
					TIM_SetCompare1(TIM14, 500);
					/*发送身份给发送任务*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					/*发送状态信息*/
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c%s%c%c%c%c%c%s%c%c",
 																		COMMAND_RESULT, COMMAND_SEPERATOR,
																		slave         , COMMAND_SEPERATOR,
 																		RES_LAMP      , COMMAND_SEPERATOR,
 																		LAMP_OFF      , COMMAND_SEPERATOR, 
 																		"0"           , COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}//end of msg!=on msg!=off
			
		}//end of server
		
		myfree(SRAMEX, auth_msg);
		myfree(SRAMEX, msg);
		myfree(SRAMEX, id_msg);
		OSTimeDlyHMSM(0,0,0,10);  //延时10ms
 	}
}

/**
 * @Function void dht11_task(void *pdata)
 * @description 处理温度和湿度的任务。
 *						收到的消息：TEMP:温度 HUMI:湿度 设备ID
 * @Input void *pdata 参数
 * @Return NULL
 */
void dht11_task(void *pdata)
{
	char * msg = NULL;
	char * msg1 = NULL;
	char * msg2 = NULL;
	u8 temp, humi;
	u8 * send_buf;
	INT8U err;
	while(1)
	{
		msg = (char *)OSQPend (dht11_event,
                0,  //wait 500ms
                &err);
		if(err != OS_ERR_NONE)
		{
				DEBUG("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
				continue; //没有接收到指令
		}
		msg1 = (char *)OSQPend (dht11_event,
                500,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("msg1 OSQPend error :%d \r\n", err);
			myfree(SRAMEX, msg);
			continue; //接收错误
		}
 		msg2 = (char *)OSQPend (dht11_event,
                 500,  //wait 2000ms
                 &err);
		if(err != OS_ERR_NONE)
		{
			DEBUG("msg2 OSQPend error %s %d\n", __FILE__, __LINE__);
			myfree(SRAMEX, msg);
			myfree(SRAMEX, msg1);
			continue; //接收错误
		}
		DEBUG("msg1:%s\n", msg1);
		DEBUG("msg2:%s\n", msg2);
		if(strcmp(msg, "client") == 0)
		{
			if(strcmp(msg1, "TEMP") == 0)
			{
				if(strcmp(msg2, "10000") == 0)
				{
					if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
					{
						continue;//获取温度失败
					}
					/*返回温度信息*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c975559549h%c"
																		"%c%c"
																			"%s%c"
																				"%c%c%c",
 																	 COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		 RES_TEMP,COMMAND_SEPERATOR,
 																		  msg2, COMMAND_SEPERATOR,  //设备号
 																		    temp, COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}

			}//end of (strcmp(msg, "TEMP") == 0)
			else if(strcmp(msg1, "HUMI") == 0)
			{
				if(strcmp(msg2, "10000") == 0)
				{
					if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
					{
						continue;//获取温度失败
					}
					/*返回湿度信息*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "client");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c975559549h%c"
																	"%c%c"
																	"%s%c"
																	"%c%c%c",
 																	COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																	RES_HUMI,COMMAND_SEPERATOR,
 			  													msg2, COMMAND_SEPERATOR,  //设备号
 																	humi, COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}//end of strcmp(msg, "HUMI") == 0
		}
		
		else if(strcmp(msg, "server") == 0)
		{
			if(strcmp(msg1, "TEMP") == 0)
			{
				if(strcmp(msg2, "10000") == 0)
				{
					if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
					{
						continue;//获取温度失败
					}
					/*返回温度信息*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c"
																		"%s%c"
																		"%c%c"
																		"%s%c"
																		"%c%c%c",
 																	 COMMAND_RESULT , COMMAND_SEPERATOR,
																	 slave          , COMMAND_SEPERATOR,
 																	 RES_TEMP       , COMMAND_SEPERATOR,
 																	 msg2           , COMMAND_SEPERATOR,  //设备号
 																	 temp           , COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}

			}//end of (strcmp(msg, "TEMP") == 0)
			else if(strcmp(msg1, "HUMI") == 0)
			{
				if(strcmp(msg2, "10000") == 0)
				{
					if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
					{
						continue;//获取温度失败
					}
					/*返回湿度信息*/
					send_buf = mymalloc(SRAMEX, 7);
					sprintf((char *)send_buf, "server");
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)//为客户端信息
					{
							DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
					
					send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
					sprintf((char *)send_buf, "%c%c%s%c"
																	"%c%c"
																	"%s%c"
																	"%c%c%c",
 																	COMMAND_RESULT,COMMAND_SEPERATOR,slave,COMMAND_SEPERATOR,
 																	RES_HUMI,COMMAND_SEPERATOR,
 			  													msg2, COMMAND_SEPERATOR,  //设备号
 																	humi, COMMAND_SEPERATOR,COMMAND_END);
					if(OSQPost(tcp_send_event,send_buf) != OS_ERR_NONE)
					{
						DEBUG("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
					}
				}
			}//end of strcmp(msg, "HUMI") == 0
		}//end of server
		
		
		
		myfree(SRAMEX, msg);
		myfree(SRAMEX, msg1);
		myfree(SRAMEX, msg2);
		OSTimeDlyHMSM(0,0,0,10);  //延时10ms
 	}
}
