/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.23
 * @filename key.c
 * @description void KEY_Init(void)(初始化KEY)
 * @FunctionList
 *		1.void KEY_Init(void);
 *    2.u8 KEY_Scan(u8 mode);
 */ 

#include "key.h"
#include "delay.h" 

/**
 * @Function void KEY_Init(void);
 * @description Init_KEY IO(初始化KEY的IO)
 *              PE2,3,4 PA0
 */
void KEY_Init(void)
{
	
	GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA|RCC_AHB1Periph_GPIOE, ENABLE);//使能GPIOA,GPIOE时钟
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4; 		//KEY0 KEY1 KEY2对应引脚
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;												//普通输入模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;									//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;												//上拉
  GPIO_Init(GPIOE, &GPIO_InitStructure);															//初始化GPIOE2,3,4
	
	 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;														//WK_UP对应引脚PA0
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN ;											//下拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);															//初始化GPIOA0
 
} 

/**
 * @Function u8 KEY_Scan(u处理8 mode);
 * @Description 按键
 *              0，没有任何按键按下
 *              1，KEY0按下
 *              2，KEY1按下
 *              3，KEY2按下 
 *              4，WKUP按下 WK_UP
 *              注意此函数有响应优先级,KEY0>KEY1>KEY2>WK_UP!!
 * @Input: mode:0,不支持连续按;1,支持连续按;
 * @Return 按键值
 */
u8 KEY_Scan(u8 mode)
{	 
	static u8 key_up=1;	//按键按松开标志
	if(mode)key_up=1;  	//支持连按		  
	if(key_up&&(KEY0==0||KEY1==0||KEY2==0||WK_UP==1))
	{
		delay_ms(10);			//去抖动 
		key_up=0;
		if(KEY0==0)return 1;
		else if(KEY1==0)return 2;
		else if(KEY2==0)return 3;
		else if(WK_UP==1)return 4;
	}else if(KEY0==1&&KEY1==1&&KEY2==1&&WK_UP==0)key_up=1; 	    
 	return 0;						// 无按键按下
}




















