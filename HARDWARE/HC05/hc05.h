#ifndef __HC05_H
#define __HC05_H	 
#include "sys.h" 

#define HC05_KEY  	PFout(6) 	//À¶ÑÀ¿ØÖÆKEYÐÅºÅ
#define HC05_LED  	PCin(0)		//À¶ÑÀÁ¬½Ó×´Ì¬ÐÅºÅ
  
u8 HC05_Init(void);
void HC05_CFG_CMD(u8 *str);
u8 HC05_Get_Role(void);
u8 HC05_Set_Cmd(u8* atstr);	   
#endif  
















