/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.25
 * @filename hc05.c
 * @description ATK-HC05
 * @FunctionList
 *		1.u8 HC05_Init(void);          //初始化
 *		2.void HC05_CFG_CMD(u8 *str);  //USMART调试
 *		3.u8 HC05_Get_Role(void);      //得到主从
 *		4.u8 HC05_Set_Cmd(u8* atstr);	 //发送AT命令给HC05
 */
#include "delay.h" 			 
#include "usart.h" 			 
#include "usart3.h" 			 
#include "hc05.h" 
#include "led.h" 
#include "string.h"	 
#include "math.h"   

/**
 * @Function u8 HC05_Init(void);
 * @description 初始化HC05
 *              初始化PC0(LED)和PF6(KEY)
 * @Return   1:检测失败
 *					 0:检测成功
 */
u8 HC05_Init(void)
{
	u8 retry=10,t;	  		 
	u8 temp=1;
	
	GPIO_InitTypeDef GPIO_InitStructure;
 
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF|RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIOC,GPIOF时钟
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; 				//LED对应引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;			//普通输入模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;			//上拉
  GPIO_Init(GPIOC, &GPIO_InitStructure);						//初始化GPIOC0
 
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;					//KEY对应引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;			//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;		//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;			//上拉
  GPIO_Init(GPIOF, &GPIO_InitStructure); 						//根据设定参数初始化PF6

	GPIO_SetBits(GPIOF,GPIO_Pin_6);
 	
	usart3_init(9600);									//初始化串口3为:9600,波特率.
	
	while(retry--)
	{
		HC05_KEY=1;												//KEY置高,进入AT模式
		delay_ms(10);
		u3_printf("AT\r\n");							//发送AT测试指令
		HC05_KEY=0;												//KEY拉低,退出AT模式
		for(t=0;t<10;t++) 								//最长等待50ms,来接收HC05模块的回应
		{
			if(USART3_RX_STA&0X8000)break;
			delay_ms(5);
		}		
		if(USART3_RX_STA&0X8000)					//接收到一次数据了
		{
			temp=USART3_RX_STA&0X7FFF;			//得到数据长度
			USART3_RX_STA=0;			 
			if(temp==4&&USART3_RX_BUF[0]=='O'&&USART3_RX_BUF[1]=='K')
			{
				temp=0;												//接收到OK响应
				break;
			}
		}			    		
	}		    
	if(retry==0)temp=1;									//检测失败
	return temp;	 
}	 

/**
 * @Function u8 HC05_Get_Role(void);
 * @description 获取ATK-HC05模块的角色
 * @Return   1:    主机
 *					 0xFF: 失败
 *					 0:    从机
 */
u8 HC05_Get_Role(void)
{	 		    
	u8 retry=0X0F;
	u8 temp,t;
	while(retry--)
	{
		HC05_KEY=1;														//KEY置高,进入AT模式
		delay_ms(10);
		u3_printf("AT+ROLE?\r\n");						//查询角色
		for(t=0;t<20;t++) 										//最长等待200ms,来接收HC05模块的回应
		{
			delay_ms(10);
			if(USART3_RX_STA&0X8000)break;
		}		
		HC05_KEY=0;														//KEY拉低,退出AT模式
		if(USART3_RX_STA&0X8000)							//接收到一次数据了
		{
			temp=USART3_RX_STA&0X7FFF;					//得到数据长度
			USART3_RX_STA=0;			 
			if(temp==13&&USART3_RX_BUF[0]=='+')//接收到正确的应答了
			{
				temp=USART3_RX_BUF[6]-'0';				//得到主从模式值
				break;
			}
		}		
	}
	if(retry==0)temp=0XFF;									//查询失败.
	return temp;
} 							   

/**
 * @Function u8 HC05_Set_Cmd(u8* atstr);
 * @description ATK-HC05设置命令
 * @Input    u8* atstr : AT指令串.比如:"AT+RESET"/"AT+UART=9600,0,0"/"AT+ROLE=0"等字符串
 * @Return   other: 失败
 *					 0:     成功
 */
u8 HC05_Set_Cmd(u8* atstr)
{	 		    
	u8 retry=0X0F;
	u8 temp,t;
	while(retry--)
	{
		HC05_KEY=1;									//KEY置高,进入AT模式
		delay_ms(10);
		u3_printf("%s\r\n",atstr);	//发送AT字符串
		HC05_KEY=0;									//KEY拉低,退出AT模式
		for(t=0;t<20;t++) 					//最长等待100ms,来接收HC05模块的回应
		{
			if(USART3_RX_STA&0X8000)break;
			delay_ms(5);
		}		
		if(USART3_RX_STA&0X8000)						//接收到一次数据了
		{
			temp=USART3_RX_STA&0X7FFF;				//得到数据长度
			USART3_RX_STA=0;			 
			if(temp==4&&USART3_RX_BUF[0]=='O')//接收到正确的应答了
			{			
				temp=0;
				break;			 
			}
		}		
	}
	if(retry==0)temp=0XFF;//设置失败.
	return temp;
} 

/**
 * @Function void HC05_CFG_CMD(u8 *str);
 * @description 通过该函数,可以利用USMART,调试接在串口3上的ATK-HC05模块
 * @Input    u8 *str : str:命令串.(这里注意不再需要再输入回车符)
 * @Return   void
 */
void HC05_CFG_CMD(u8 *str)
{					  
	u8 temp;
	u8 t;		  
	HC05_KEY=1;											//KEY置高,进入AT模式
	delay_ms(10);
	u3_printf("%s\r\n",(char*)str); //发送指令
	for(t=0;t<50;t++) 							//最长等待500ms,来接收HC05模块的回应
	{
		if(USART3_RX_STA&0X8000)break;
		delay_ms(10);
	}									    
	HC05_KEY=0;												//KEY拉低,退出AT模式
	if(USART3_RX_STA&0X8000)					//接收到一次数据了
	{
		temp=USART3_RX_STA&0X7FFF;			//得到数据长度
		USART3_RX_STA=0;
		USART3_RX_BUF[temp]=0;					//加结束符		 
		printf("\r\n%s",USART3_RX_BUF);//发送回应数据到串口1
	} 				 
}














