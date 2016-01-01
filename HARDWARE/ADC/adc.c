/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.26
 * @filename adc.c
 * @description ADC
 * @FunctionList
 *		1.void Adc_Init(void); 								//ADC通道初始化
 *		2.u16  Get_Adc(u8 ch); 								//获得某个通道值 
 *		3.u16 Get_Adc_Average(u8 ch,u8 times);//得到某个通道给定次数采样的平均值  
 *		4.short Get_Temprate(void);           //得到内部温度
 */ 
#include "adc.h"
#include "delay.h"		 

/**
 * @Function void  Adc_Init(void)
 * @description 初始化ADC	
 * @Input 	void
 * @Return  void
 */
void  Adc_Init(void)
{    
  GPIO_InitTypeDef  GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef       ADC_InitStructure;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE); //使能ADC1时钟

  //先初始化ADC1通道5 IO口
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;							//PA5 通道5
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;					//模拟输入
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ;			//不带上下拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);								//初始化  
 
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,ENABLE);	  //ADC1复位
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,DISABLE);	//复位结束	 
 
	ADC_TempSensorVrefintCmd(ENABLE);                     //开启内部温度传感器控制位
	
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;								//独立模式
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;//两个采样阶段之间的延迟5个时钟
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //DMA失能
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4;							//预分频4分频。ADCCLK=PCLK2/4=84/4=21Mhz,ADC时钟最好不要超过36Mhz 
  ADC_CommonInit(&ADC_CommonInitStructure);																//初始化
	
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12位模式
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;					//非扫描模式	
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;		//关闭连续转换
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//禁止触发检测，使用软件触发
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//右对齐	
  ADC_InitStructure.ADC_NbrOfConversion = 1;						//1个转换在规则序列中 也就是只转换规则序列1 
  ADC_Init(ADC1, &ADC_InitStructure);										//ADC初始化
	
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_480Cycles);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_480Cycles); //ADC16,ADC通道,480个周期,提高采样时间可以提高精度
 
	ADC_Cmd(ADC1, ENABLE);//开启AD转换器	

}				  

/**
 * @Function u16 Get_Adc(u8 ch) 
 * @description 获得ADC值
 * @Input 	u8: 通道值 0~16取值范围为：ADC_Channel_0~ADC_Channel_16
 * @Return  u16:转换结果
 */
u16 Get_Adc(u8 ch)   
{
	u16 wait = 1000;
  //设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );	//ADC1,ADC通道,480个周期,提高采样时间可以提高精确度			    
  
	ADC_SoftwareStartConv(ADC1);										//使能指定的ADC1的软件转换启动功能	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC )&& wait) wait--;	//等待转换结束

	if(wait > 0)return ADC_GetConversionValue(ADC1);						//返回最近一次ADC1规则组的转换结果
	return 0;
}

/**
 * @Function u16 Get_Adc_Average(u8 ch,u8 times)
 * @description 获取通道ch的转换值，取times次,然后平均 
 * @Input 	u8: 通道值 0~16取值范围为：ADC_Channel_0~ADC_Channel_16
 *					U8: times:获取次数
 * @Return  u16:通道ch的times次转换结果平均值
 */
u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
} 

/**
 * @Function short Get_Temprate(void)
 * @description 得到内部温度
 * @Input 	void
 * @Return  short:温度值(扩大100倍,单位：°C)
 */
short Get_Temprate(void)
{
	u32 adcx;
	short result;
 	double temperate;
	adcx=Get_Adc_Average(ADC_Channel_16,10);	//读取通道16，10次
	temperate=(float)adcx*(3.3/4096);		//电压值
	temperate=(temperate-0.76)/0.0025 + 25; //转换为温度 
	result=temperate*=100;					//扩大100倍
	return result;
}
	 









