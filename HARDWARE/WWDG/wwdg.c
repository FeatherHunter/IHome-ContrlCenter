/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.23
 * @filename wwdg.c
 * @description 窗口看门狗相关设置
 * @FunctionList
 *		1.void WWDG_Init(u8 tr,u8 wr,u32 fprer); 窗口看门狗初始化
 *    2.void WWDG_IRQHandler(void);            中断服务函数
 *    
 */ 
#include "wwdg.h"
#include "led.h"


//保存WWDG计数器的设置值,默认为最大. 
u8 WWDG_CNT=0X7F; 	

/**
 * @Function void WWDG_Init(u8 tr,u8 wr,u32 fprer);
 * @description 初始化窗口看门狗
 *							Fwwdg=PCLK1/(4096*2^fprer). 一般PCLK1=42Mhz
 * @Input:  u8  tr   :T[6:0],计数器值 
 *					u8  wr   :W[6:0],窗口值 
 *          u32 fprer:分频系数（WDGTB）,仅最低2位有效 
 * @Return void
 *
 */
void WWDG_Init(u8 tr,u8 wr,u32 fprer)
{
 
	NVIC_InitTypeDef NVIC_InitStructure;
 
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG,ENABLE); //使能窗口看门狗时钟
	
	WWDG_CNT=tr&WWDG_CNT;   														//初始化WWDG_CNT. 
	WWDG_SetPrescaler(fprer); 													//设置分频值
	WWDG_SetWindowValue(wr); 														//设置窗口值
//	WWDG_SetCounter(WWDG_CNT);//设置计数值
	WWDG_Enable(WWDG_CNT);  														//开启看门狗
	
	NVIC_InitStructure.NVIC_IRQChannel=WWDG_IRQn;  			//窗口看门狗中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02;  //抢占优先级为2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;					//子优先级为3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;  							//使能窗口看门狗
	NVIC_Init(&NVIC_InitStructure);
	
	WWDG_ClearFlag();																		//清除提前唤醒中断标志位
  WWDG_EnableIT();																		//开启提前唤醒中断
}

/**
 * @Function void WWDG_IRQHandler(void);
 * @description 窗口看门狗中断服务程序 
 */
void WWDG_IRQHandler(void)
{
	WWDG_SetCounter(WWDG_CNT); //重设窗口看门狗值
	WWDG_ClearFlag();					//清除提前唤醒中断标志位
	LED1=!LED1;
}
