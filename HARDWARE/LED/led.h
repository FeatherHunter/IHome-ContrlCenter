#ifndef __LED_H
#define __LED_H
#include "sys.h"

/*      LED�˿ڶ���     */
#define LED0 PFout(9)		// DS0
#define LED1 PFout(10)	// DS1
#define LEDW PBout(1)  //LED WARM

void LED_Init(void);		//��ʼ��		 				    
#endif
