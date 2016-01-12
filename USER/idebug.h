#ifndef _H_DEBUG
#define _H_DEBUG

#define _DEBUG 1

#if _DEBUG
	#define DEBUG(format, ...) printf (format, ##__VA_ARGS__)
	#define DEBUG_LCD(x,y,z,c) POINT_COLOR = c;LCD_ShowString(x,y,200,20,16,z)
#else
  #define DEBUG(format, ...)
	#define DEBUG_LCD(x,y,z,c)
#endif

#endif
