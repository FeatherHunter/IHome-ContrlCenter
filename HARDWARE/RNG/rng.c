#include "rng.h"
#include "delay.h"
/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.23
 * @filename rng.c
 * @description 硬件随机数发生器
 * @FunctionList
 *		1.u8 RNG_Init(void);
 *		2.u32 RNG_Get_RandomNum(void)
 *		3.int RNG_Get_RandomRange(int min,int max)
 */ 

/**
 * @Function u8 RNG_Init(void);
 * @Description Init_RNG
 * @Return 0:SUCESS
 *				 1:Error 
 */
u8 RNG_Init(void)
{
	u16 retry=0; 
	
  RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);//开启RNG时钟,来自PLL48CLK
	
	RNG_Cmd(ENABLE);	//使能RNG
	
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)==RESET&&retry<10000)	//等待随机数就绪
	{
		retry++;
		delay_us(100);
	}
	if(retry>=10000)return 1;//随机数产生器工作不正常
	return 0;
}

/**
 * @Function u32 RNG_Get_RandomNum(void);
 * @Description Get_RandomNum
 * @Return u32 得到随机数
 */
u32 RNG_Get_RandomNum(void)
{	 
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)==RESET);	//等待随机数就绪  
	return RNG_GetRandomNumber();	
}

/**
 * @Function int RNG_Get_RandomRange(int min,int max);
 * @Description Get_RandomRange
 * @Input: int min: 下限
 *         int max: 上限
 * @Return u32 得到随机数
 */
int RNG_Get_RandomRange(int min,int max)
{ 
   return RNG_Get_RandomNum()%(max-min+1) +min;
}



