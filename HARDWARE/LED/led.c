/**
 * @copyright ������(2014~2024) QQ��975559549
 * @Author Feather @version 1.0 @date 2015.11.23
 * @filename led.c
 * @description Init_LED(��ʼ��LED)
 * @FunctionList
 *		1.void LED_Init(void);
 */ 
 
#include "led.h" 	 

/**
 * @Function void LED_Init(void);
 * @description Init_LED IO(��ʼ��LED��IO)
 *              ��ʼ��PF9��PF10Ϊ�����,��ʹ�ܶ˿�ʱ��
 */
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOB, ENABLE);		//ʹ��GPIOBʱ��

  /*                  GPIOF9,F10��ʼ������            */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14;	//LED0��LED1��ӦIO��
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;						//��ͨ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//����
  GPIO_Init(GPIOF, &GPIO_InitStructure);									//��ʼ��GPIO
	
	GPIO_SetBits(GPIOF,GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14);						//GPIOF9,F10���øߣ�����
	
	/*                  GPIOB0��ʼ������            */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;	//ů�ƶ�ӦIO��
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;						//��ͨ���ģʽ
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;					//�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;			//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;						//����
  GPIO_Init(GPIOB, &GPIO_InitStructure);									//��ʼ��GPIO
	GPIO_ResetBits(GPIOB,GPIO_Pin_1);
}

