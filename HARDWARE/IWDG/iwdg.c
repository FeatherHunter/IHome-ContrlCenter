/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.23
 * @filename iwdg.c
 * @description 独立看门狗相关设置
 * @FunctionList
 *		1.void IWDG_Init(u8 prer,u16 rlr);
 *    2.void IWDG_Feed(void)
 *    
 */ 
 
#include "iwdg.h"

/**
 * @Function void IWDG_Init(u8 prer,u16 rlr);
 * @description 初始化独立看门狗
 *							分频因子=4*2^prer.但最大值只能是256!
 * 							时间计算(大概):Tout=((4*2^prer)*rlr)/32 (ms).
 * @Input: u8 prer  分频数:0~7(只有低3位有效!)
 *         u16 rlr  自动重装载值,0~0XFFF,低11位有效.
 * @Return void
 *
 */
void IWDG_Init(u8 prer,u16 rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); //使能对IWDG->PR IWDG->RLR的写
	
	IWDG_SetPrescaler(prer); 										  //设置IWDG分频系数

	IWDG_SetReload(rlr);     											//设置IWDG装载值

	IWDG_ReloadCounter();												  //reload
	
	IWDG_Enable();       													//使能看门狗
}

/**
 * @Function void IWDG_Feed(void);
 * @description 喂独立看门狗
 */
void IWDG_Feed(void)
{
	IWDG_ReloadCounter();//reload
}
