/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.28
 * @filename 24cxx.c
 * @description 24cxx EEPROM
 * @FunctionList
 *		1.u8 AT24CXX_ReadOneByte(u16 ReadAddr);														//指定地址读取一个字节
 *		2.void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite);				//指定地址写入一个字节
 *		3.void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len);//指定地址开始写入指定长度的数据
 *		4.u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len);										//指定地址开始读取指定长度数据
 *		5.void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite);		//从指定地址开始写入指定长度的数据
 *		6.void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead);   		//从指定地址开始读出指定长度的数据

 *		7.u8 AT24CXX_Check(void);  																				//检查器件
 *		8.void AT24CXX_Init(void); 																				//初始化IIC
 */ 
#include "24cxx.h" 
#include "delay.h" 				 

//初始化IIC接口
void AT24CXX_Init(void)
{
	IIC_Init();//IIC初始化
}

/**
 * @Function u8 AT24CXX_ReadOneByte(u16 ReadAddr);
 * @description 在AT24CXX指定地址读出一个数据
 * @Input 	u16 ReadAddr:开始读数的地址 
 * @Return  u8  读到的数据
 */
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;		  	    																 
  IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	  		  //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);		//发送高地址	    
	}else IIC_Send_Byte(0XA0+((ReadAddr/256)<<1));   //发送器件地址0XA0,写数据 	   
	IIC_Wait_Ack(); 
  IIC_Send_Byte(ReadAddr%256);    //发送低地址
	IIC_Wait_Ack();	    
	IIC_Start();  	 	   
	IIC_Send_Byte(0XA1);            //进入接收模式			   
	IIC_Wait_Ack();	 
  temp=IIC_Read_Byte(0);		   
  IIC_Stop();											//产生一个停止条件	    
	return temp;
}

/**
 * @Function void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite);
 * @description 在AT24CXX指定地址写入一个数据
 * @Input 	u16 WriteAddr  :写入数据的目的地址  
 *          u8  DataToWrite:要写入的数据
 * @Return  void
 */
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{				   	  	    																 
  IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	    		//发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);	//发送高地址	  
	}else IIC_Send_Byte(0XA0+((WriteAddr/256)<<1));   //发送器件地址0XA0,写数据 	 
	IIC_Wait_Ack();	   
  IIC_Send_Byte(WriteAddr%256);   //发送低地址
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //发送字节							   
	IIC_Wait_Ack();  		    	   
  IIC_Stop();											//产生一个停止条件 
	delay_ms(10);	 
}

/**
 * @Function void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
 * @description 在AT24CXX里面的指定地址开始写入长度为Len的数据
 *              该函数用于写入16bit或者32bit的数据.
 * @Input 	u16 WriteAddr  :写入数据的目的地址  
 *          u8  DataToWrite:要写入的数据
 *					u8  Len:要写入数据的长度2,4
 * @Return  void
 */
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

/**
 * @Function u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
 * @description 在AT24CXX里面的指定地址开始读出长度为Len的数据
 *							该函数用于读出16bit或者32bit的数据.
 * @Input 	u16 ReadAddr   :开始读出的地址 
 *					u8  Len        :要读出数据的长度2,4
 * @Return  u8 :数据
 */
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
{  	
	u8 t;
	u32 temp=0;
	for(t=0;t<Len;t++)
	{
		temp<<=8;
		temp+=AT24CXX_ReadOneByte(ReadAddr+Len-t-1); 	 				   
	}
	return temp;												    
}

/**
 * @Function u8 AT24CXX_Check(void)
 * @description 检查AT24CXX是否正常
 *					这里用了24XX的最后一个地址(255)来存储标志字.
 *					如果用其他24C系列,这个地址要修改
 * @Input 	void
 * @Return  1:检测失败 
 *          0:检测成功
 */
u8 AT24CXX_Check(void)
{
	u8 temp;
	temp=AT24CXX_ReadOneByte(255);//避免每次开机都写AT24CXX			   
	if(temp==0X55)return 0;		   
	else//排除第一次初始化的情况
	{
		AT24CXX_WriteOneByte(255,0X55);
	    temp=AT24CXX_ReadOneByte(255);	  
		if(temp==0X55)return 0;
	}
	return 1;											  
}


/**
 * @Function void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
 * @description 在AT24CXX里面的指定地址开始读出指定个数的数据
 * @Input 	u16 ReadAddr :开始读出的地址 对24c02为0~255
 *					u8 *pBuffer  :数据数组首地址
 *					u16 NumToRead:要读出数据的个数
 * @Return  void
 */
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  
 
/**
 * @Function void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
 * @description 在AT24CXX里面的指定地址开始写入指定个数的数据
 * @Input 	u16 WriteAddr :开始写入的地址 对24c02为0~255
 *					u8 *pBuffer  :数据数组首地址
 *					u16 NumToWrite:要写入数据的个数
 * @Return  void
 */
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}








