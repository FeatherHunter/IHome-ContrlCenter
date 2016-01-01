/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.23
 * @filename beep.c
 * @description Init_BEEP(初始化BEEP)
 * @FunctionList
 *		1.void BEEP_Init(void)
 */ 	
#include "beep.h"  

/**
 * @Function void BEEP_Init(void)
 * @description Init_BEEP IO(初始化BEEP的IO)
 *              初始化PF8为输出口,并使能端口时钟
 */
void BEEP_Init(void)
{   
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);	//使能GPIOF时钟
  
  /*    初始化蜂鸣器对应引脚GPIOF8  */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;					//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;				//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;		//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;				//下拉
  GPIO_Init(GPIOF, &GPIO_InitStructure);								//初始化GPIO
	
  GPIO_ResetBits(GPIOF,GPIO_Pin_8);  										//蜂鸣器对应引脚GPIOF8拉低， 
}






