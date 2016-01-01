#ifndef _WWDG_H
#define _WWDG_H
#include "sys.h"

void WWDG_Init(u8 tr,u8 wr,u32 fprer); //窗口看门狗初始化
void WWDG_IRQHandler(void);            //窗口看门狗中断
#endif
