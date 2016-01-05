#ifndef _H_DEBUG
#define _H_DEBUG

#define _DEBUG 0

#if _DEBUG
	#define DEBUG(format, ...) printf (format, ##__VA_ARGS__)
#else
  #define DEBUG(format, ...)
#endif

#endif
