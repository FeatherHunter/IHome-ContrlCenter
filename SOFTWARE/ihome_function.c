#include "ihome_function.h"
#include "task_priority.h"
#include "led.h"

//led任务
void led_task(void *pdata)
{
	char * msg = NULL;
	u8 * send_buf;
	INT8U err;
	
	while(1)
	{
		printf("led task!\n");
		msg = (char *)OSQPend (led_event,
                0,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			printf("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
			continue; //接收错误
		}
		if(strcmp(msg, "ON") == 0)
		{
 			myfree(SRAMEX, msg);
			msg = (char *)OSQPend (led_event,
                2000,  //wait 2000ms
                &err);
 			if(strcmp(msg, "0") == 0)
 			{
 				LEDW = 1;
				/*发送状态信息*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
				sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
																		RES_LAMP,COMMAND_SEPERATOR,
																		LAMP_ON, COMMAND_SEPERATOR, 
																		"0", COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
 				tcp_client_flag|=1<<7;//标记要发送数据
 			}
		}
 		else if(strcmp(msg, "OFF") == 0)
 		{
 			myfree(SRAMEX, msg);
 			msg = (char *)OSQPend (led_event,
                 2000,  //wait 2000ms
                 &err);
			if(strcmp(msg, "0") == 0)
 			{
 				LEDW = 0;
				/*发送状态信息*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
				sprintf((char *)send_buf, "%c%c975559549h%c%c%c%c%c%s%c%c",
 																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		RES_LAMP,COMMAND_SEPERATOR,
 																		LAMP_OFF, COMMAND_SEPERATOR, 
 																		"0", COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
 				tcp_client_flag|=1<<7;//标记要发送数据
 			}
 		}
		myfree(SRAMEX, msg);
		OSTimeDlyHMSM(0,0,0,10);  //延时10ms
 	}
}

void dht11_task(void *pdata)
{
	char * msg = NULL;
	int temp, humi;
	u8 * send_buf;
	INT8U err;
	
	while(1)
	{
		printf("dht11 task!\n");
		msg = (char *)OSQPend (dht11_event,
                0,  //wait forever
                &err);
		if(err != OS_ERR_NONE)
		{
			printf("LED TASK OSQPend error %s %d\n", __FILE__, __LINE__);
			continue; //接收错误
		}
		if((strcmp(msg, "TMEP") == 0)&&(strcmp(msg, "HUMI") == 0))
		{
			myfree(SRAMEX, msg);
 			msg = (char *)OSQPend (led_event,
                 2000,  //wait 2000ms
                 &err);
			if(strcmp(msg, "10000") == 0)
 			{
				if(DHT11_Read_Data((u8*)&temp, (u8*)&humi)==1)
				{
					continue;//获取温度失败
				}
				/*返回温度信息*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
				sprintf((char *)send_buf, "%c%c975559549h%c%c%c%s%c%d%c%c",
 																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		RES_TEMP,COMMAND_SEPERATOR,
 																		msg, COMMAND_SEPERATOR,  //设备号
 																		temp, COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
				/*返回湿度信息*/
				send_buf = mymalloc(SRAMEX, 74); //32(account)+32(ID)+9+1 状态指令最高上限
				sprintf((char *)send_buf, "%c%c975559549h%c%c%c%s%c%d%c%c",
 																		COMMAND_RESULT,COMMAND_SEPERATOR,COMMAND_SEPERATOR,
 																		RES_HUMI,COMMAND_SEPERATOR,
 																		msg, COMMAND_SEPERATOR,  //设备号
 																		humi, COMMAND_SEPERATOR,COMMAND_END);
				if(OSQPost(msg_event,send_buf) != OS_ERR_NONE)
				{
					printf("OSQPost ERROR %s %d\n", __FILE__, __LINE__);
				}
 				tcp_client_flag|=1<<7;//标记要发送数据
 			}

		}
		myfree(SRAMEX, msg);
		OSTimeDlyHMSM(0,0,0,10);  //延时10ms
 	}
}
