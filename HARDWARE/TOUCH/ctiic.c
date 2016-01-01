/**
 * @copyright 王辰浩(2014~2024) QQ：975559549
 * @Author Feather @version 1.0 @date 2015.11.27
 * @filename ctiic.c
 * @description ctiic
 * @FunctionList
 *		1.void CT_IIC_Init(void);                	//初始化IIC的IO口				 
 *		2.void CT_IIC_Start(void);								//发送IIC开始信号
 *		3.void CT_IIC_Stop(void);	  							//发送IIC停止信号
 *		4.void CT_IIC_Send_Byte(u8 txd);					//IIC发送一个字节
 *		5.u8 CT_IIC_Read_Byte(unsigned char ack);	//IIC读取一个字节
 *		6.u8 CT_IIC_Wait_Ack(void); 							//IIC等待ACK信号
 *		7.void CT_IIC_Ack(void);									//IIC发送ACK信号
 *		8.void CT_IIC_NAck(void);									//IIC不发送ACK信号 
 */ 
#include "ctiic.h"
#include "delay.h"	 


//控制I2C速度的延时
void CT_Delay(void)
{
	delay_us(2);
} 

/**
 * @Function void CT_IIC_Init(void);
 * @description 电容触摸芯片IIC接口初始化
 *							PB0,PF11
 * @Input 	void
 * @Return  void
 */	
void CT_IIC_Init(void)
{			
  GPIO_InitTypeDef  GPIO_InitStructure;	
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOF, ENABLE);//使能GPIOB,F时钟
	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;			//PB0设置为推挽输出
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	//上拉
  GPIO_Init(GPIOB, &GPIO_InitStructure);				//初始化
		
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;		//PF11设置推挽输出
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;	//输出模式
	GPIO_Init(GPIOF, &GPIO_InitStructure);				//初始化	

}


/**
 * @Function void CT_IIC_Start(void)
 * @description 产生IIC起始信号
 * @Input 	void
 * @Return  void
 */	
void CT_IIC_Start(void)
{
	CT_SDA_OUT();    //sda线输出
	CT_IIC_SDA=1;	  	  
	CT_IIC_SCL=1;
	delay_us(30);
 	CT_IIC_SDA=0;		//START:when CLK is high,DATA change form high to low 
	CT_Delay();
	CT_IIC_SCL=0;		//钳住I2C总线，准备发送或接收数据 
}	  

/**
 * @Function void CT_IIC_Stop(void)
 * @description 产生IIC停止信号
 * @Input 	void
 * @Return  void
 */	
void CT_IIC_Stop(void)
{
	CT_SDA_OUT();//sda线输出
	CT_IIC_SCL=1;
	delay_us(30);
	CT_IIC_SDA=0;//STOP:when CLK is high DATA change form low to high
	CT_Delay();
	CT_IIC_SDA=1;//发送I2C总线结束信号  
}

/**
 * @Function u8 CT_IIC_Wait_Ack(void)
 * @description 等待应答信号到来
 * @Input  void
 * @Return 	1，接收应答失败
 *          0，接收应答成功
 */	
u8 CT_IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;
	CT_SDA_IN();      //SDA设置为输入  
	CT_IIC_SDA=1;	   
	CT_IIC_SCL=1;
	CT_Delay();
	while(CT_READ_SDA)
	{
		ucErrTime++;
		if(ucErrTime>250)
		{
			CT_IIC_Stop();
			return 1;
		} 
		CT_Delay();
	}
	CT_IIC_SCL=0;//时钟输出0 	   
	return 0;  
} 

/**
 * @Function void CT_IIC_Ack(void)
 * @description 产生ACK应答
 * @Input 	void
 * @Return  void
 */	
void CT_IIC_Ack(void)
{
	CT_IIC_SCL=0;
	CT_SDA_OUT();
	CT_Delay();
	CT_IIC_SDA=0;
	CT_Delay();
	CT_IIC_SCL=1;
	CT_Delay();
	CT_IIC_SCL=0;
}

/**
 * @Function void CT_IIC_NAck(void)
 * @description 不产生ACK应答		
 * @Input 	void
 * @Return  void
 */	
void CT_IIC_NAck(void)
{
	CT_IIC_SCL=0;
	CT_SDA_OUT();
	CT_Delay();
	CT_IIC_SDA=1;
	CT_Delay();
	CT_IIC_SCL=1;
	CT_Delay();
	CT_IIC_SCL=0;
}					 				     
	
/**
 * @Function void CT_IIC_Send_Byte(u8 txd)
 * @description IIC发送一个字节
 * @Input 	void
 * @Return  1，有应答
 *					0，无应答	
 */	
void CT_IIC_Send_Byte(u8 txd)
{                        
  u8 t;   
	CT_SDA_OUT(); 	    
  CT_IIC_SCL=0;//拉低时钟开始数据传输
	CT_Delay();
	for(t=0;t<8;t++)
  {              
    CT_IIC_SDA=(txd&0x80)>>7;
    txd<<=1; 	      
		CT_IIC_SCL=1; 
		CT_Delay();
		CT_IIC_SCL=0;	
		CT_Delay();
  }	 
} 	    
/**
 * @Function u8 CT_IIC_Read_Byte(unsigned char ack)
 * @description IIC读1个字节
 * @Input 	ack=1时，发送ACK，ack=0，发送nACK
 * @Return  u8 接收到数据
 */	
u8 CT_IIC_Read_Byte(unsigned char ack)
{
	u8 i,receive=0;
 	CT_SDA_IN();//SDA设置为输入
	delay_us(30);
	for(i=0;i<8;i++ )
	{ 
		CT_IIC_SCL=0; 	    	   
		CT_Delay();
		CT_IIC_SCL=1;	 
		receive<<=1;
		if(CT_READ_SDA)receive++;   
	}	  				 
	if (!ack)CT_IIC_NAck();//发送nACK
	else CT_IIC_Ack(); //发送ACK   
 	return receive;
}





























