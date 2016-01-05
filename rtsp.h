#ifndef _TT_RTSP_H_
#define _TT_RTSP_H_

#include <stddefs.h>

BOOL rtsp_open (U16 PortNo,char* server);
BOOL rtsp_play(U32 npt);
BOOL rtsp_pause(void);
BOOL rtsp_stop(void);
U32 rtsp_getparam(void);

#endif
