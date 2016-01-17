Copyright 版权所有 @IFuture Technology
本项目是IFuture科技公司(intelligent future technolgy)旗下IHome(智能家庭)项目中家庭控制终端项目
基于stm32,移植了ucosII和lwip,开发平台keil

1.项目负责人:feather(王辰浩)
2.项目文件指引:
  1.USER           main.c、
		   instructions.h   自定义通信协议指令
		   idebug.h	    调试BUG宏
  2.LWIP/lwip_app  tcp_client.c     stm32作为客户端连接外网服务器
	           tcp_server.c     stm32作为wifi内网中的服务器
  3.SOFTWARE       ihome_function.c 控制终端各种设备的控制任务
  4.HARDWARE       各种硬件设备的初始化以及使用

3.版本信息与功能介绍:

v2.0  @Date:2015/12/26  移植ucosII和lwip,使用RAW编程接口。实现基本登录,灯,温湿度,智能家居模式
v2.10 @Date:2016/1/8    重做架构，采用NETCONN编程接口。使用自定义通信协议,提升指令解析和网络通信效率。降低控制延时。
v2.30 @Date:2016/1/14   增加TCP服务器端，用于手机在wifi模式下直接连接stm32


4.工作完成的代码量

4755行

feather（王辰浩）贡献代码量：3250行

