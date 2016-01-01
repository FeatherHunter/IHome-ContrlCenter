/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.27
 * @filename lsens.c
 * @description Light Sensor
 * @FunctionList
 *		1.void Lsens_Init(void); 				//初始化光敏传感器
 *		2.u8 Lsens_Get_Val(void);				//读取光敏传感器的值
 */ 
 
#include "lsens.h"
#include "delay.h"
  
/**
 * @Function void Lsens_Init(void)
 * @description 初始化光敏传感器
 * @Input 	void
 * @Return  void
 */	
void Lsens_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//使能GPIOF时钟
	
  //先初始化ADC3通道7IO口
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;					//PA7 通道7
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;			//模拟输入
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;	//不带上下拉
  GPIO_Init(GPIOF, &GPIO_InitStructure);						//初始化  
 
	Adc3_Init();//初始化ADC3
}
/**
 * @Function u8 Lsens_Get_Val(void);
 * @description 读取Light Sens的值
 * @Input 	void
 * @Return  u8:0~100:0,最暗;100,最亮
 */	
u8 Lsens_Get_Val(void)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<LSENS_READ_TIMES;t++)
	{
		temp_val+=Get_Adc3(ADC_Channel_5);	//读取ADC值,通道5
		delay_ms(5);
	}
	temp_val/=LSENS_READ_TIMES;//得到平均值 
	if(temp_val>4000)temp_val=4000;
	return (u8)(100-(temp_val/40));
}












