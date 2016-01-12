#ifndef _H_CAMERA
#define _H_CAMERA
#include "sys.h"
/**
 * @copyright 王辰浩(2016~2026) QQ：975559549
 * @Author Feather @version 1.0 @date 2016.1.12
 * @filename camera.h
 * @description 摄像头的头文件，具体实现参照camera.c
 * @FunctionList
 *		1.void camera_Init(void);                       //初始化
 *		2.void sw_ov2640_mode(void);                    //切换为OV2640模式（GPIOC8/9/11切换为 DCMI接口）
 *		3.void sw_sdcard_mode(void);                    //切换为SD卡模式（GPIOC8/9/11切换为 SDIO接口）
 *		4.void jpeg_data_process(void); 							  //处理JPEG数据
 *		5.u8 ov2640_jpg_photo(u8 *pname);  						  //OV2640拍照jpg图片
 *		6.void jpeg_dcmi_rx_callback(void); 						//jpeg数据接收回调函数
 *		7.void camera_new_pathname(u8 *pname,u8 mode);  //文件名自增（避免覆盖）
 */ 
 
extern u8 *pname;				//带路径的文件名 
extern int camera_server_isready;

extern u8 ov2640_mode;						        //工作模式:0,RGB565模式;1,JPEG模式

#define jpeg_dma_bufsize	5*1024		     //定义JPEG DMA接收时数据缓存jpeg_buf0/1的大小(*4字节)
extern volatile u32 jpeg_data_len; 			 //buf中的JPEG有效数据长度(*4字节)
extern volatile u8 jpeg_data_ok;				 //JPEG数据采集完成标志 
										//0,数据没有采集完;
										//1,数据采集完了,但是还没处理;
										//2,数据已经处理完成了,可以开始下一帧接收
										
extern u32 *jpeg_buf0;							//JPEG数据缓存buf,通过malloc申请内存
extern u32 *jpeg_buf1;							//JPEG数据缓存buf,通过malloc申请内存
extern u32 *jpeg_data_buf;				  //JPEG数据缓存buf,通过malloc申请内存

void camera_Init(void);                       //初始化
void sw_ov2640_mode(void);                    //切换为OV2640模式（GPIOC8/9/11切换为 DCMI接口）
void sw_sdcard_mode(void);                    //切换为SD卡模式（GPIOC8/9/11切换为 SDIO接口）
void jpeg_data_process(void); 							  //处理JPEG数据
u8 ov2640_jpg_photo(u8 *pname);  						  //OV2640拍照jpg图片
void jpeg_dcmi_rx_callback(void); 						//jpeg数据接收回调函数
void camera_new_pathname(u8 *pname,u8 mode);  //文件名自增（避免覆盖）

#endif
