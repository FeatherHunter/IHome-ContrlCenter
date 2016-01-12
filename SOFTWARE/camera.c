/**
 * @copyright ������(2016~2026) QQ��975559549
 * @Author Feather @version 1.0 @date 2016.1.12
 * @filename camera.c
 * @description ����ͷ��ع��ܣ�������JPEGͼƬ��
 * @FunctionList
 *		1.void camera_Init(void);                       //��ʼ��
 *		2.void sw_ov2640_mode(void);                    //�л�ΪOV2640ģʽ��GPIOC8/9/11�л�Ϊ DCMI�ӿڣ�
 *		3.void sw_sdcard_mode(void);                    //�л�ΪSD��ģʽ��GPIOC8/9/11�л�Ϊ SDIO�ӿڣ�
 *		4.void jpeg_data_process(void); 							  //����JPEG����
 *		5.u8 ov2640_jpg_photo(u8 *pname);  						  //OV2640����jpgͼƬ
 *		6.void jpeg_dcmi_rx_callback(void); 						//jpeg���ݽ��ջص�����
 *		7.void camera_new_pathname(u8 *pname,u8 mode);  //�ļ������������⸲�ǣ�
 */ 
#include "camera.h"
#include "main.h"
#include "sys.h"
#include "delay.h"

u8 *pname;				//��·�����ļ��� 
int camera_server_isready = 0; //�ͻ����Ѿ��������˷����� 

u8 ov2640_mode=0;						//����ģʽ:0,RGB565ģʽ;1,JPEGģʽ

#define jpeg_dma_bufsize	5*1024		//����JPEG DMA����ʱ���ݻ���jpeg_buf0/1�Ĵ�С(*4�ֽ�)
volatile u32 jpeg_data_len=0; 			//buf�е�JPEG��Ч���ݳ���(*4�ֽ�)
volatile u8 jpeg_data_ok=0;				//JPEG���ݲɼ���ɱ�־ 
										//0,����û�вɼ���;
										//1,���ݲɼ�����,���ǻ�û����;
										//2,�����Ѿ����������,���Կ�ʼ��һ֡����
										
u32 *jpeg_buf0;							//JPEG���ݻ���buf,ͨ��malloc�����ڴ�
u32 *jpeg_buf1;							//JPEG���ݻ���buf,ͨ��malloc�����ڴ�
u32 *jpeg_data_buf;				  //JPEG���ݻ���buf,ͨ��malloc�����ڴ�

/**
 * @Function void camera_Init(void);
 * @description init camera
 *       1. malloc space for jpeg dma
 *			 2. init OV2640 and set mode = JPEG
 */
void camera_Init(void)
{
	jpeg_buf0=mymalloc(SRAMIN,jpeg_dma_bufsize*4);	//Ϊjpeg dma���������ڴ�	
	jpeg_buf1=mymalloc(SRAMIN,jpeg_dma_bufsize*4);	//Ϊjpeg dma���������ڴ�	
	jpeg_data_buf=mymalloc(SRAMEX,300*1024);		//Ϊjpeg�ļ������ڴ�(���300KB)
 	pname=mymalloc(SRAMIN,30);//Ϊ��·�����ļ�������30���ֽڵ��ڴ�	 
 	while(pname==NULL||!jpeg_buf0||!jpeg_buf1||!jpeg_data_buf)	//�ڴ�������
 	{	 
    DEBUG_LCD(30,190,"jpeg malloc failed", RED);		
		delay_ms(200);				  
		LCD_Fill(30,190,240,146,WHITE);//�����ʾ	     
		delay_ms(200);				  
	}   
	while(OV2640_Init())//��ʼ��OV2640
	{
		DEBUG_LCD(30,190,"OV2640 ERROR!    ", RED);	
		delay_ms(200);
	    LCD_Fill(30,190,239,206,WHITE);
		delay_ms(200);
	}	
	DEBUG_LCD(30,190,"OV2640 SUCCESS!    ", GREEN);	
	delay_ms(2000);
	OV2640_RGB565_Mode();	//JPEGģʽ
}

/**
 * @Function void jpeg_data_process(void)
 * @description ����JPEG����
 *		���ɼ���һ֡JPEG���ݺ�,���ô˺���,�л�JPEG BUF.��ʼ��һ֡�ɼ�.
 */
void jpeg_data_process(void)
{
	u16 i;
	u16 rlen;//ʣ�����ݳ���
	u32 *pbuf;
	if(ov2640_mode)//ֻ����JPEG��ʽ��,����Ҫ������.
	{
		if(jpeg_data_ok==0)	//jpeg���ݻ�δ�ɼ���
		{
			DMA_Cmd(DMA2_Stream1,DISABLE);		//ֹͣ��ǰ����
			while(DMA_GetCmdStatus(DMA2_Stream1) != DISABLE);	//�ȴ�DMA2_Stream1������ 
			rlen=jpeg_dma_bufsize-DMA_GetCurrDataCounter(DMA2_Stream1);//�õ�ʣ�����ݳ���	
			pbuf=jpeg_data_buf+jpeg_data_len;//ƫ�Ƶ���Ч����ĩβ,�������
			if(DMA2_Stream1->CR&(1<<19))for(i=0;i<rlen;i++)pbuf[i]=jpeg_buf1[i];//��ȡbuf1�����ʣ������
			else for(i=0;i<rlen;i++)pbuf[i]=jpeg_buf0[i];//��ȡbuf0�����ʣ������ 
			jpeg_data_len+=rlen;			//����ʣ�೤��
			jpeg_data_ok=1; 				//���JPEG���ݲɼ��갴��,�ȴ�������������
		}
		if(jpeg_data_ok==2)	//��һ�ε�jpeg�����Ѿ���������
		{ DMA_SetCurrDataCounter(DMA2_Stream1,jpeg_dma_bufsize);//���䳤��Ϊjpeg_buf_size*4�ֽ�
			DMA_Cmd(DMA2_Stream1,ENABLE); //���´���
			jpeg_data_ok=0;					//�������δ�ɼ�
			jpeg_data_len=0;				//�������¿�ʼ
		}
	}
}

/**
 * @Function void jpeg_dcmi_rx_callback(void)
 * @description jpeg���ݽ��ջص�����
 */
void jpeg_dcmi_rx_callback(void)
{ 
	u16 i;
	u32 *pbuf;
	pbuf=jpeg_data_buf+jpeg_data_len;//ƫ�Ƶ���Ч����ĩβ
	if(DMA2_Stream1->CR&(1<<19))     //buf0����,��������buf1
	{ 
		for(i=0;i<jpeg_dma_bufsize;i++)pbuf[i]=jpeg_buf0[i];//��ȡbuf0���������
		jpeg_data_len+=jpeg_dma_bufsize;//ƫ��
	}else //buf1����,��������buf0
	{
		for(i=0;i<jpeg_dma_bufsize;i++)pbuf[i]=jpeg_buf1[i];//��ȡbuf1���������
		jpeg_data_len+=jpeg_dma_bufsize;//ƫ�� 
	} 	
}
/**
 * @Function void sw_ov2640_mode(void)
 * @description �л�ΪOV2640ģʽ��GPIOC8/9/11�л�Ϊ DCMI�ӿڣ�
 */
void sw_ov2640_mode(void)
{
	OV2640_PWDN=0;//OV2640 Power Up
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_DCMI);  //PC8,AF13  DCMI_D2
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_DCMI);  //PC9,AF13  DCMI_D3
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_DCMI); //PC11,AF13 DCMI_D4  
 
} 
/**
 * @Function void sw_sdcard_mode(void)
 * @description �л�ΪSD��ģʽ��GPIOC8/9/11�л�Ϊ SDIO�ӿڣ�
 */
void sw_sdcard_mode(void)
{
	OV2640_PWDN=1;//OV2640 Power Down  
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_SDIO);  //PC8,AF12
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_SDIO);//PC9,AF12 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_SDIO); 
}
/**
 * @Function void camera_new_pathname(u8 *pname,u8 mode)
 * @description �ļ������������⸲�ǣ�
 *              bmp��ϳ�:����"0:PHOTO/PIC13141.bmp"���ļ���
 *							jpg��ϳ�:����"0:PHOTO/PIC13141.jpg"���ļ���
 * @Input: u8 *pname; //file path name
 *         u8  mode; 0,����.bmp�ļ�;1,����.jpg�ļ�.
 */
void camera_new_pathname(u8 *pname,u8 mode)
{	 
	u8 res;					 
	u16 index=0;
	while(index<0XFFFF)
	{
		if(mode==0)sprintf((char*)pname,"0:PHOTO/PIC%05d.bmp",index);
		else sprintf((char*)pname,"0:PHOTO/PIC%05d.jpg",index);
		res=f_open(ftemp,(const TCHAR*)pname,FA_READ);//���Դ�����ļ�
		if(res==FR_NO_FILE)break;		//���ļ���������=����������Ҫ��.
		index++;
	}
} 
/**
 * @Function u8 ov2640_jpg_photo(u8 *pname)
 * @description OV2640����jpgͼƬ
 * @Input:  u8 *pname;  file path name
 * @Return  0    : success
 *          other: failed
 */
 u8 ov2640_jpg_photo(u8 *pname)
{
	FIL* f_jpg;
	u8 res=0;
	u32 bwr;
	u16 i;
	u8* pbuf;
	f_jpg=(FIL *)mymalloc(SRAMIN,sizeof(FIL));	//����FIL�ֽڵ��ڴ����� 
	if(f_jpg==NULL)return 0XFF;									//�ڴ�����ʧ��.
	ov2640_mode=1;
	sw_ov2640_mode();														//�л�ΪOV2640ģʽ
	dcmi_rx_callback=jpeg_dcmi_rx_callback;			//�ص�����
	DCMI_DMA_Init((u32)jpeg_buf0,(u32)jpeg_buf1,jpeg_dma_bufsize,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA����(˫����ģʽ)
	OV2640_JPEG_Mode();												//�л�ΪJPEGģʽ 
 	OV2640_ImageWin_Set(0,0, 1600, 1200);			 
	OV2640_OutSize_Set(1600, 1200);						//���ճߴ�Ϊ1600*1200
	DCMI_Start(); 										//�������� 
	while(jpeg_data_ok!=1);						//�ȴ���һ֡ͼƬ�ɼ���
	jpeg_data_ok=2;										//���Ա�֡ͼƬ,������һ֡�ɼ�
	while(jpeg_data_ok!=1);						//�ȴ��ڶ�֡ͼƬ�ɼ���
	jpeg_data_ok=2;										//���Ա�֡ͼƬ,������һ֡�ɼ�
	while(jpeg_data_ok!=1);						//�ȴ�����֡ͼƬ�ɼ���,����֡,�ű��浽SD��ȥ.
	DCMI_Stop(); 											//ֹͣDMA����
	ov2640_mode=0;
	sw_sdcard_mode();									//�л�ΪSD��ģʽ
	res=f_open(f_jpg,(const TCHAR*)pname,FA_WRITE|FA_CREATE_NEW);//ģʽ0,���߳��Դ�ʧ��,�򴴽����ļ�	 
	if(res==0)
	{
		printf("jpeg data size:%d\r\n",jpeg_data_len*4);//���ڴ�ӡJPEG�ļ���С
		pbuf=(u8*)jpeg_data_buf;
		for(i=0;i<jpeg_data_len*4;i++)//����0XFF,0XD8
		{
			if((pbuf[i]==0XFF)&&(pbuf[i+1]==0XD8))break;
		}
		if(i==jpeg_data_len*4)res=0XFD;//û�ҵ�0XFF,0XD8
		else//�ҵ���
		{
			pbuf+=i;//ƫ�Ƶ�0XFF,0XD8��
			res=f_write(f_jpg,pbuf,jpeg_data_len*4-i,&bwr);
			if(bwr!=(jpeg_data_len*4-i))res=0XFE; 
		}
	}
	jpeg_data_len=0;
	f_close(f_jpg); 
	sw_ov2640_mode();		//�л�ΪOV2640ģʽ
	OV2640_RGB565_Mode();	//RGB565ģʽ 
	DCMI_DMA_Init((u32)&LCD->LCD_RAM,0,1,DMA_MemoryDataSize_HalfWord,DMA_MemoryInc_Disable);//DCMI DMA����           
	myfree(SRAMIN,f_jpg);
  printf("%x\r\n", res);	
	return res;
}

/**
 * @Function u8 ov2640_jpg_photo(u8 *pname)
 * @description OV2640����jpgͼƬ
 * @Input:  u8 *pname;  file path name
 * @Return  0    : success
 *          other: failed
 */
 u8 ov2640_jpg_send(u8 *pname)
{
	FIL* f_jpg;
	u8 res=0;
	u32 bwr;
	u16 i;
	u8* pbuf;

	ov2640_mode=1; //JPEGģʽ
	sw_ov2640_mode();														//�л�ΪOV2640ģʽ
	OV2640_JPEG_Mode();												//�л�ΪJPEGģʽ 
 	OV2640_ImageWin_Set(0,0, 1600, 1200);			 
	OV2640_OutSize_Set(1600, 1200);						//���ճߴ�Ϊ1600*1200
	DCMI_Start(); 										//�������� 
	while(jpeg_data_ok!=1);						//�ȴ���һ֡ͼƬ�ɼ���
	jpeg_data_ok=2;										//���Ա�֡ͼƬ,������һ֡�ɼ�
	while(jpeg_data_ok!=1);						//�ȴ��ڶ�֡ͼƬ�ɼ���
	jpeg_data_ok=2;										//���Ա�֡ͼƬ,������һ֡�ɼ�
	while(jpeg_data_ok!=1);						//�ȴ�����֡ͼƬ�ɼ���,����֡,�ű��浽SD��ȥ.
	DCMI_Stop(); 											//ֹͣDMA����

	printf("jpeg data size:%d\r\n",jpeg_data_len*4);//���ڴ�ӡJPEG�ļ���С
	pbuf=(u8*)jpeg_data_buf;
	for(i=0;i<jpeg_data_len*4;i++)//����0XFF,0XD8
	{
			if((pbuf[i]==0XFF)&&(pbuf[i+1]==0XD8))break;
	}
	if(i==jpeg_data_len*4)res=0XFD;//û�ҵ�0XFF,0XD8
	else//�ҵ���
	{
		pbuf+=i;//ƫ�Ƶ�0XFF,0XD8��
		res=f_write(f_jpg,pbuf,jpeg_data_len*4-i,&bwr);
		if(bwr!=(jpeg_data_len*4-i))res=0XFE; 
	}
	jpeg_data_len=0;

	sw_ov2640_mode();		//�л�ΪOV2640ģʽ
        
	myfree(SRAMIN,f_jpg);
  printf("%x\r\n", res);	
	return res;
}


