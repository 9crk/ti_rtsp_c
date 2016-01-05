/*******************************************************************************
 *             Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
 *                           ALL RIGHTS RESERVED
 * FileName: networksvr.c
 * Description: 
 *
 * History:
 * Version   Date         Author     DefectNum    Description
 * main1     2008-09-01   diaoqiangwei/d60770                
 ******************************************************************************/
#include "yh_stapp_main.h"
#include "Mid_dvm_cmd.h"
#include "lwip/sockets.h" 

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define MAX_BUFF_NUM 20
#define MAX_BUFF_SIZE 1500

static S32 sock_fd = -1;
static Task_t s_pIPTaskHandle = NULL;		/*networksvr task handle*/

static STFDMA_GenericNode_t *FDMA_Node,*FDMA_NodeKernel,*FDMA_NodeDevice;
static STFDMA_ChannelId_t  FDMA_ChannelId;
static STFDMA_Pool_t       FDMA_Pool;
static STFDMA_Block_t      FDMA_Block;
static STFDMA_TransferId_t FDMA_TransferId; 
static U32	FDMA_MemoryType;
static STFDMA_TransferGenericParams_t FDMA_TransferParams;

static U32 g_AvInjectBufferId =0;
static void* g_AvInjectBuffer[MAX_BUFF_NUM];

void AvInjectData(U8 *buf, int size, int offset)
{
	U32 TS_BufferSrcBase;	
	U32 TS_BufferSrcBaseDevice;
	ST_ErrorCode_t ErrCode;

	if (buf == NULL || size <= 0) return;
	
	TS_BufferSrcBase = (U32)buf;

	TS_BufferSrcBaseDevice = (U32)SYS_Memory_UserToDevice(FDMA_MemoryType, TS_BufferSrcBase);

	FDMA_Node->Node.SourceAddress_p = (void *)TS_BufferSrcBaseDevice + offset;
	FDMA_Node->Node.Length          = size;
	FDMA_Node->Node.NumberBytes     = size;
	
	ErrCode = STFDMA_StartGenericTransfer(&FDMA_TransferParams, &FDMA_TransferId, TRUE);
	if (ErrCode != ST_NO_ERROR)
	{
		printf("--> Unable to perform the AV fdma transfer !\n");
	}
}

ST_ErrorCode_t InitAvInject(void)
{
	int i;
	ST_ErrorCode_t ErrCode;

	g_AvInjectBufferId =0;
	FDMA_MemoryType = SYS_MEMORY_NCACHED|SYS_MEMORY_CONTIGUOUS;

	for (i=0; i<MAX_BUFF_NUM;i++)
	{
		g_AvInjectBuffer[i] = (U8 *)SYS_Memory_Allocate(FDMA_MemoryType, MAX_BUFF_SIZE, 32);	
	}
	
	FDMA_Node = (STFDMA_GenericNode_t *)SYS_Memory_Allocate(FDMA_MemoryType, sizeof(STFDMA_GenericNode_t), 256);
	if (FDMA_Node == NULL)
	{
		printf("--> Unable to allocate memory for a FDMA node !\n");
		return(ST_ERROR_NO_MEMORY);
	}
	
	memset(FDMA_Node, 0, sizeof(STFDMA_Node_t));
	FDMA_NodeDevice = (STFDMA_GenericNode_t *)(SYS_Memory_UserToDevice(FDMA_MemoryType, (U32)FDMA_Node));
	FDMA_NodeKernel = (STFDMA_GenericNode_t *)(SYS_Memory_UserToKernel(FDMA_MemoryType, (U32)FDMA_Node));

	/* Lock a dedicated channel */
	/* ------------------------ */
	FDMA_Pool  = STFDMA_DEFAULT_POOL;
	FDMA_Block = STFDMA_2;
	
	ErrCode = STFDMA_LockChannelInPool(FDMA_Pool, &FDMA_ChannelId, FDMA_Block);
	if (ErrCode != ST_NO_ERROR)
	{
		SYS_Memory_Deallocate(FDMA_MemoryType, (U32)FDMA_Node);
		printf("--> Unable to lock a FDMA channel in the pool !\n");
		return (ErrCode);
	}

	FDMA_TransferParams.ApplicationData_p = NULL;
	FDMA_TransferParams.BlockingTimeout   = 0;
	FDMA_TransferParams.CallbackFunction  = NULL;
	FDMA_TransferParams.Pool              = FDMA_Pool;
	FDMA_TransferParams.ChannelId         = FDMA_ChannelId;
	FDMA_TransferParams.NodeAddress_p     = (void *)(FDMA_NodeDevice);
	FDMA_TransferParams.NodeAddress_Vp    = (void *)(FDMA_NodeKernel);
	FDMA_TransferParams.FDMABlock         = FDMA_Block;
	
	FDMA_Node->Node.DestinationAddress_p             = (void *)(ST7105_TSMERGE2_BASE_ADDRESS);
	FDMA_Node->Node.SourceStride                     = 0; 
	FDMA_Node->Node.DestinationStride                = 0;
	FDMA_Node->Node.Next_p                           = NULL;
#if defined(DVD_SECURED_CHIP)
	FDMA_Node->Node.NodeControl.Secure	             = 1;
#endif
	FDMA_Node->Node.NodeControl.SourceDirection      = STFDMA_DIRECTION_INCREMENTING;
	FDMA_Node->Node.NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
	FDMA_Node->Node.NodeControl.NodeCompleteNotify   = TRUE;
	FDMA_Node->Node.NodeControl.NodeCompletePause    = FALSE;	
	FDMA_Node->Node.NodeControl.PaceSignal = STFDMA_REQUEST_SIGNAL_SWTS0;

	return ST_NO_ERROR;
}

static U16 s_u16LastSequenceNum = 0xffff;

static int FillAVBuffer(void)
 {
	U8 u8First;
	U16 u16ExtLen;
	U32 u32Offset, u32Len;
	BOOL u8Extension;
	U16 u16SequenceNum = 0;
	U8* u8Tmp;
	S32 s32Len, s32AddrLen;
	struct sockaddr_in server_addr;

	int ret;
	fd_set read_set;
	struct timeval timeout;	
	timeout.tv_sec = 0;
 	timeout.tv_usec = 500000; 

	FD_ZERO(&read_set);
	FD_SET(sock_fd, &read_set);

	ret = select(sock_fd + 1, &read_set, NULL, NULL, &timeout); 
	if (ret <= 0)
	{			
		return -1;
	}
	
	if (!FD_ISSET(sock_fd, &read_set))return -1;
	
	s32AddrLen = sizeof(struct sockaddr_in);
	s32Len = recvfrom(sock_fd, g_AvInjectBuffer[g_AvInjectBufferId], MAX_BUFF_SIZE, 0, (struct sockaddr *)&server_addr, &s32AddrLen);
	if (s32Len <= 0)
	{		
		return -1;
	}
	
	u8Tmp =  g_AvInjectBuffer[g_AvInjectBufferId];
	g_AvInjectBufferId ++;
	if(g_AvInjectBufferId >= MAX_BUFF_NUM)
	{
		g_AvInjectBufferId = 0;
	}
	
#if 1
	u8First = u8Tmp[0];
	if ((u8First & 0x80) != 0x80)
	{
		printf("First Byte is 0x%2x\n", u8First);
		return -1;
	}

	u16SequenceNum = (U16)(u8Tmp[2] << 8) | (U16)u8Tmp[3];
	if (u16SequenceNum == s_u16LastSequenceNum)
	{		
		return -1;
	}	

	s_u16LastSequenceNum = u16SequenceNum;
	u8Extension = (u8First >> 4) & 0x01;
	u32Offset = 12;
    
	//有扩展部分
	if (u8Extension == TRUE)
	{
	    u16ExtLen  = (U16)(u8Tmp[14] << 8) | u8Tmp[15];
	    u32Offset += ((u16ExtLen << 2) + 4);
	}
	u32Len = s32Len - u32Offset;

	AvInjectData(u8Tmp, u32Len, u32Offset);
#else
	AvInjectData(u8Tmp , s32Len, 0); 
#endif	

	return  0;
}

/*****************************************************************************
 * Function:      IPTaskProc
 * Description:   
 * Data Accessed:
 * Data Updated: 
 * Input:             None
 * Output:           None
 * Return:  None
 * Others:     None
 *****************************************************************************/
static void IPTaskProc(void)
{
	/* 消息处理*/	
	while (1)
	{
		FillAVBuffer();
		//FillTS_Test();
	}
}

ST_ErrorCode_t IPTV_Play(int PTI_ID)
{
	int i;
	BOOL PID_Found;
	ST_ErrorCode_t ErrCode;
	Pids_Info_t	pidsInfo;
	STTAPI_PATData_t          PAT;
	STTAPI_PMTData_t          PMT;
 
	/* Get the PAT now */
	ErrCode=STTAPI_PAT_Acquire(PTI_ID,STTAPI_PAT_PID,&PAT);
	if (ErrCode!=ST_NO_ERROR)
	{
		printf("--> Unable to get the PAT !\n");
		return ErrCode;
	}

	if(PAT.NbElements < 1)
	{
		STTAPI_PAT_Delete(&PAT);
		return ST_ERROR_BAD_PARAMETER;
	}

	PID_Found=FALSE;
	//for(i = 0; i < PAT.NbElements; i ++)
	for(i = PAT.NbElements -1; i >=0 ; i --)
	{
		/* Get the PMT for this program */
		memset(&PMT,0,sizeof(STTAPI_PMTData_t));
		PMT.ProgramNumber=PAT.Element[i].ProgramNumber;
		ErrCode=STTAPI_PMT_Acquire(PTI_ID,PAT.Element[i].Pid,&PMT);
		if (ErrCode ==ST_NO_ERROR)
		{
			PID_Found = TRUE;
			break;
		}
	}

	if(PID_Found == FALSE)
	{
		STTAPI_PAT_Delete(&PAT);
		return(ST_ERROR_BAD_PARAMETER);
	}
	
	if (PMT.NbElements==0)
	{
		 printf("--> This program is empty !\n");
		 STTAPI_PMT_Delete(&PMT);
		 STTAPI_PAT_Delete(&PAT);
		 return(ST_ERROR_BAD_PARAMETER);
	}
	
	/* Get the list of pids */
	for (i=0,pidsInfo.PidsNum=0;i<PMT.NbElements;i++)
	{
		PID_Found=FALSE;
		pidsInfo.Pids[pidsInfo.PidsNum].Pid  = PMT.Element[i].Pid;
		switch(PMT.Element[i].Type)
		{
		case STTAPI_STREAM_VIDEO_MPEG1 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_MP1V;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_VIDEO_MPEG2 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_MP2V;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_VIDEO_MPEG4 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_H264;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_VIDEO_VC1 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_VC1;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_VIDEO_AVS :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_AVS;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_MPEG1 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_MP1A;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_MPEG2 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_MP2A;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_WMA :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_WMA;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_AAC_ADTS :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_AAC;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_AAC_LATM :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_HEAAC;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_AAC_RAW1 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_HEAAC;	
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_AAC_RAW2 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_HEAAC;
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_LPCM :
			break;
		case STTAPI_STREAM_AUDIO_MLP  :
			break;
		case STTAPI_STREAM_AUDIO_AC3 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_AC3;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_DTS       :
		case STTAPI_STREAM_AUDIO_DTSHD     :
		case STTAPI_STREAM_AUDIO_DTSHD_XLL :
		case STTAPI_STREAM_AUDIO_DTSHD_2   :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_DTS;			
			PID_Found=TRUE;
			break;
		case STTAPI_STREAM_AUDIO_DDPLUS   :
		case STTAPI_STREAM_AUDIO_DDPLUS_2 :
			pidsInfo.Pids[pidsInfo.PidsNum].Type = PLAYREC_STREAMTYPE_DDPLUS;
			PID_Found=TRUE;
			break;
		default :
			break;
		}	
	
		if (PID_Found==TRUE) pidsInfo.PidsNum++;
		if (pidsInfo.PidsNum==PLAYREC_MAX_PIDS)
		{			
			break;
		}
	}
	
	/* Free the table elements */
	STTAPI_PMT_Delete(&PMT);
	STTAPI_PAT_Delete(&PAT);

	ErrCode = Mid_DVM_PlayLive(&pidsInfo, PTI_ID, 0);
	
	if (ErrCode != ST_NO_ERROR)
	{
		return ErrCode;
	}
	return ST_NO_ERROR;	
}

int IPTV_Start(U16 port)
{
	ST_ErrorCode_t ErrCode;
	struct sockaddr_in peerAddr;
	struct ip_mreq mreq;
	
	if (sock_fd != -1)
	{
		close(sock_fd);
		sock_fd = -1;
	}

	sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_fd < 0)
	{		
		return -1;
	}		
	
	bzero(&peerAddr, sizeof(struct sockaddr_in));
	peerAddr.sin_family=AF_INET;
	peerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	peerAddr.sin_port = htons(port);
		
	if (bind(sock_fd, (struct sockaddr *)&peerAddr, sizeof(struct sockaddr_in)) < 0)
	{	
		close(sock_fd);
		sock_fd = -1;
        	return -1;
	}
	
	s_pIPTaskHandle = Task_Create(IPTaskProc, NULL, 16 * 1024, 10, "IP Task", 0, s_pIPTaskHandle);
	if (NULL == s_pIPTaskHandle)
	{
		printf("networksvr task create failed!\n");
		return NULL;
	}

	Task_Priority_Set(s_pIPTaskHandle, 10);
	
	return 0;
}

ST_ErrorCode_t IPTV_Setup(void)
{
	ST_ErrorCode_t ErrCode;
	
	ErrCode = InitAvInject();	
	if(ErrCode != ST_NO_ERROR)
	{
		return ErrCode;
	}

	Task_Delay(ST_GetClocksPerSecond( )*2);
	while(1)
	{
		if(rtsp_open(5004, "rtsp://172.16.17.1/Lie_to_me_02_Cut_001.ts"))break;
	}
	
	rtsp_play(0);
	IPTV_Start(5004);	
	
	ErrCode = IPTV_Play(2);
	if(ErrCode != ST_NO_ERROR)
	{
		printf("IPTV_Play error\n");
		return ErrCode;
	}
	
	printf("IPTV_Play ok\n");

	while(1)
	{
		Task_Delay(ST_GetClocksPerSecond()*5);
		rtsp_getparam();
	}
	
	return ST_NO_ERROR;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


