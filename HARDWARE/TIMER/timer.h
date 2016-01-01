#ifndef _TIMER_H
#define _TIMER_H
#include "sys.h"

extern u8  TIM5CH1_CAPTURE_STA;		//输入捕获状态		    				
extern u32	TIM5CH1_CAPTURE_VAL;	//输入捕获值 

void TIM3_Int_Init(u16 arr,u16 psc); //定时器3初始化
void TIM7_Int_Init(u16 arr,u16 psc); //定时器7初始化
void TIM5_CH1_Cap_Init(u32 arr,u16 psc);
void TIM14_PWM_Init(u32 arr,u32 psc);
#endif
