#ifndef _H_CAMERA
#define _H_CAMERA
#include "sys.h"
/**
 * @copyright ������(2016~2026) QQ��975559549
 * @Author Feather @version 1.0 @date 2016.1.12
 * @filename camera.h
 * @description ����ͷ��ͷ�ļ�������ʵ�ֲ���camera.c
 * @FunctionList
 *		1.void camera_Init(void);                       //��ʼ��
 *		2.void sw_ov2640_mode(void);                    //�л�ΪOV2640ģʽ��GPIOC8/9/11�л�Ϊ DCMI�ӿڣ�
 *		3.void sw_sdcard_mode(void);                    //�л�ΪSD��ģʽ��GPIOC8/9/11�л�Ϊ SDIO�ӿڣ�
 *		4.void jpeg_data_process(void); 							  //����JPEG����
 *		5.u8 ov2640_jpg_photo(u8 *pname);  						  //OV2640����jpgͼƬ
 *		6.void jpeg_dcmi_rx_callback(void); 						//jpeg���ݽ��ջص�����
 *		7.void camera_new_pathname(u8 *pname,u8 mode);  //�ļ������������⸲�ǣ�
 */ 
 
extern u8 *pname;				//��·�����ļ��� 
extern int camera_server_isready;

extern u8 ov2640_mode;						        //����ģʽ:0,RGB565ģʽ;1,JPEGģʽ

#define jpeg_dma_bufsize	5*1024		     //����JPEG DMA����ʱ���ݻ���jpeg_buf0/1�Ĵ�С(*4�ֽ�)
extern volatile u32 jpeg_data_len; 			 //buf�е�JPEG��Ч���ݳ���(*4�ֽ�)
extern volatile u8 jpeg_data_ok;				 //JPEG���ݲɼ���ɱ�־ 
										//0,����û�вɼ���;
										//1,���ݲɼ�����,���ǻ�û����;
										//2,�����Ѿ����������,���Կ�ʼ��һ֡����
										
extern u32 *jpeg_buf0;							//JPEG���ݻ���buf,ͨ��malloc�����ڴ�
extern u32 *jpeg_buf1;							//JPEG���ݻ���buf,ͨ��malloc�����ڴ�
extern u32 *jpeg_data_buf;				  //JPEG���ݻ���buf,ͨ��malloc�����ڴ�

void camera_Init(void);                       //��ʼ��
void sw_ov2640_mode(void);                    //�л�ΪOV2640ģʽ��GPIOC8/9/11�л�Ϊ DCMI�ӿڣ�
void sw_sdcard_mode(void);                    //�л�ΪSD��ģʽ��GPIOC8/9/11�л�Ϊ SDIO�ӿڣ�
void jpeg_data_process(void); 							  //����JPEG����
u8 ov2640_jpg_photo(u8 *pname);  						  //OV2640����jpgͼƬ
void jpeg_dcmi_rx_callback(void); 						//jpeg���ݽ��ջص�����
void camera_new_pathname(u8 *pname,u8 mode);  //�ļ������������⸲�ǣ�

#endif
