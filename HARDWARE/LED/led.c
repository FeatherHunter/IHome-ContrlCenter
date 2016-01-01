/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.23
 * @filename led.c
 * @description Init_LED(初始化LED)
 * @FunctionList
 *		1.void LED_Init(void);
 */ 
 
#include "led.h" 	 

/**
 * @Function void LED_Init(void);
 * @description Init_LED IO(初始化LED的IO)
 *              初始化PF9和PF10为输出口,并使能端口时钟
 */
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);		//使能GPIOF时钟

  /*                  GPIOF9,F10初始化设置            */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_15;	//LED0和LED1对应IO口
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;						//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//上拉
  GPIO_Init(GPIOF, &GPIO_InitStructure);									//初始化GPIO
	
	GPIO_SetBits(GPIOF,GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_15);						//GPIOF9,F10设置高，灯灭

}






