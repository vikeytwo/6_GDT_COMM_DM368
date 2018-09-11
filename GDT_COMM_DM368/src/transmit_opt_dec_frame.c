/******************************************************************************
 	Program         :	GDT_COMM_DM368 of Transmit Decode Composite  (for DM368)
 	Author           :	ZhangJ
	Modification	:	2018-07 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#include "system_config.h"
#include "transmit_interface.h"

/*Composite Data Info*/
#define FRAME_HEAD						{0x1A, 0xCF, 0xFC, 0x1D}
#define FRAME_HEAD_SIZE				4
#define FRAME_HEAD_DATA_SIZE		24

/*Pick Frame Head*/
#define MATCH_SIZE						((FRAME_HEAD_SIZE-1)*2)

/*L-Link Frame Data (A)*/
#define LA_LINK_LEN						1024
#define LA_LINK_HEAD_LEN				10
#define POS_LA_FRAME_TYPE			4
#define POS_LA_FRAME_LEN				5
#define POS_LA_VIDEO_LSB				6
#define POS_LA_VIDEO_MSB			7

/*L-Link Frame Data (B)*/
#define LB_LINK_LEN						(4+2+72)
#define LB_LINK_HEAD_LEN				(4+2+4)
#define POS_LB_FRAME_HEAD			(4+2)
#define POS_LB_FRAME_TYPE			(4+2+2)
#define POS_LB_FRAME_LEN				(4+2+3)

/*U-Link Frame Data*/
#define U_LINK_LEN						72
#define U_LINK_HEAD_LEN				4
#define POS_U_FRAME_TYPE			2
#define POS_U_FRAME_LEN				3

/*Common Frame Data*/
#define HOLDER_LEN						7
#define STATION_MEAS_LEN				32
#define L_FRAME_TYPE_NUM			4
#define U_FRAME_TYPE_NUM			5
#define MAX_DATA_LEN1014			1014
#define MAX_DATA_LEN68				68

/*Reserve Test*/ 
int temp; 

static void socket_init(Socket_Info_S arrOpt[])
{
	int i, s32Ret;
	Socket_Info_S	 *pSockOpt = arrOpt;

	for(i=0; i<OUTPUT_DATA_CNT; i++){
	 /*(pSockOpt+i)->selCastFlag		=	MULTICAST_SEND;
		(pSockOpt+i)->bindLocalFlag	=	BIND_LOCAL_IP;
		(pSockOpt+i)->pLocalAddr		=	LOCAL_ETH0_IP;
		(pSockOpt+i)->localPort			=  LOCAL_L_PLANE_PORT + (i*2);
		(pSockOpt+i)->pMcastAddr		=	PARSE_DATA_IP;
		(pSockOpt+i)->mcastPort			=  OUTPUT_L_PLANE_PORT + (i*2); 
		s32Ret = init_udp_socket(pSockOpt+i);*/

	 /*(*(pSockOpt+i)).selCastFlag		=	MULTICAST_SEND;
		(*(pSockOpt+i)).bindLocalFlag	=	BIND_LOCAL_IP;
		(*(pSockOpt+i)).pLocalAddr		=	LOCAL_ETH0_IP;
		(*(pSockOpt+i)).localPort			=  LOCAL_L_PLANE_PORT + (i*2);
		(*(pSockOpt+i)).pMcastAddr		=	PARSE_DATA_IP;
		(*(pSockOpt+i)).mcastPort		=  OUTPUT_L_PLANE_PORT + (i*2); 
		s32Ret = init_udp_socket(pSockOpt+i);*/
		arrOpt[i].selCastFlag		=	MULTICAST_SEND;
		arrOpt[i].bindLocalFlag		=	BIND_LOCAL_IP;
		arrOpt[i].pLocalAddr			=	LOCAL_ETH0_IP;
		arrOpt[i].localPort			=  LOCAL_L_PLANE_PORT + (i*2);
		arrOpt[i].pMcastAddr		=	PARSE_DATA_IP;
		arrOpt[i].mcastPort			=  OUTPUT_L_PLANE_PORT + (i*2); 
		s32Ret = init_udp_socket(&arrOpt[i]);
		if(s32Ret < 0){
			printf("[%s]%d: Init Socket Error, [%d]\n",__FILE__, __LINE__, i);
			exit(EXIT_FAILURE);
		}
	}
}

/*Parse Complex Data A (Complex_A = FE + Holder + LStatMeas + URemoMeas + LStatMeas)*/
static void parse_complex_data_a(Socket_Info_S	arrOpt[], unsigned char *pBuffer, int length)
{
	unsigned char *pBuf = (unsigned char *)pBuffer;
	int s32Len = length;
	int i, s32SendLen=0, s32PassLen=0;
	int s32LikFraType, s32LikFraLen, s32VidFlag, s32VidLen;

#if 0  /* FHA: Frame Cnt*/	
	/* Reserve: Detect Whether Lost Valid Frame */
	if(((pBuf[9]<<8) | pBuf[8])==0){
		temp=0;
	}else{
		if((((pBuf[9]<<8) | pBuf[8])-temp) != 1){
			printf("[%s]%d:new****** last=%d , current=%d \n",__FILE__,__LINE__,temp, ((pBuf[9]<<8) | pBuf[8]));
			if(temp!=0){
				//exit(0);
			}
		}
		temp = (pBuf[9]<<8) | pBuf[8];
	}	
#endif

	/*1.Send L-Link Frame (L-Plane/L-Load/L-UpLikMeas/U-UpLikMeas)*/
	s32LikFraType = pBuf[POS_LA_FRAME_TYPE] & 0x0F;
	s32LikFraLen = pBuf[POS_LA_FRAME_LEN];	
	if((s32LikFraType!=0x00)&&(s32LikFraLen<=MAX_DATA_LEN1014)){
		for(i=0; i<L_FRAME_TYPE_NUM; i++){
			if((s32LikFraType>>i)==0x01){
				i += NUM_L_PLANE;
				s32SendLen = send_udp_socket_data(&arrOpt[i], pBuf+LA_LINK_HEAD_LEN, s32LikFraLen);
				if(s32SendLen != s32LikFraLen)
					printf("[%s]%d: Send Down Link Frame Error, [%d]\n",__FILE__, __LINE__, i);
				break;
			}
		}
	}else{
		s32LikFraLen = 0;
	}

	/*2.Send Video Data*/
	s32VidFlag = pBuf[POS_LA_FRAME_TYPE]>>5;
	s32VidLen = (pBuf[POS_LA_VIDEO_MSB]<<8) | pBuf[POS_LA_VIDEO_LSB];
	if((s32VidFlag != 0)&&(s32VidLen<=(MAX_DATA_LEN1014-s32LikFraLen))){
		for(i=0; i<VIDEO_NUM; i++){
			if((s32VidFlag>>i)==0x01){	
				pthread_mutex_lock(&videoMut[i]);
				if(writeFlag[i] != INVALID){
					memcpy(pVideoBuf[i]+s32VideoLen[i], pBuf+LA_LINK_HEAD_LEN+s32LikFraLen, s32VidLen);
					s32VideoLen[i] = s32VideoLen[i] + s32VidLen;
				}
				pthread_cond_signal(&videoCond[i]);
				pthread_mutex_unlock(&videoMut[i]);
				break;
			}
		}
	}
	s32PassLen += LA_LINK_LEN;

	/*3.Send Holder Report*/
	s32SendLen = send_udp_socket_data(&arrOpt[NUM_HOLDER], pBuf+s32PassLen, HOLDER_LEN);
	if(s32SendLen != HOLDER_LEN)
		printf("[%s]%d: Send Holder Error, %d]\n",__FILE__, __LINE__, NUM_HOLDER);
	s32PassLen += HOLDER_LEN;

	/*4.Send L Station Meas*/
	if((pBuf[s32PassLen]==0xEB)&&(pBuf[s32PassLen+1]==0x90)){
		s32SendLen = send_udp_socket_data(&arrOpt[NUM_L_STAT_MEAS], pBuf+s32PassLen, STATION_MEAS_LEN);
		if(s32SendLen != STATION_MEAS_LEN)
			printf("[%s]%d: Send L Station Meas Error, [%d]\n",__FILE__, __LINE__, NUM_L_STAT_MEAS);
	}
	s32PassLen += STATION_MEAS_LEN;

	//printf("0x%02x, 0x%02x, 0x%02x ,s32PassLen=%d \n",pBuf[s32PassLen],pBuf[s32PassLen+1],pBuf[s32PassLen+2],s32PassLen);
	/*5.Send U-Link Frame (U-Plane/U-Load/U-UpLikMeas/U-UpLikMeas)*/
	if((pBuf[s32PassLen]==0x09)&&(pBuf[s32PassLen+1]==0xD7)){
		s32LikFraType = pBuf[s32PassLen+POS_U_FRAME_TYPE]&0x0F; 
		if(s32LikFraType != 0x00){
			s32LikFraLen = pBuf[s32PassLen+POS_U_FRAME_LEN];		/*(0~68)*/
			for(i=0;i<U_FRAME_TYPE_NUM;i++){
				if((s32LikFraType>>i)==0x01){
					i+=NUM_U_PLANE;
					s32SendLen = send_udp_socket_data(&arrOpt[i], pBuf+s32PassLen+U_LINK_HEAD_LEN, s32LikFraLen);
					if(s32SendLen != s32LikFraLen)
						printf("[%s]%d: Send Down Link Frame Error, [%d]\n",__FILE__, __LINE__, i);
					break;
				}
			}
		}
	}
	s32PassLen += U_LINK_LEN;

	/*6.Send U Station Meas*/
	//if((pBuf[s32PassLen]==0xEB)&&(pBuf[s32PassLen+1]==0x90)){
	//	s32SendLen = send_udp_socket_data(&arrOpt[NUM_U_STAT_MEAS], pBuf+s32PassLen, STATION_MEAS_LEN);
	//	if(s32SendLen != STATION_MEAS_LEN)
	//		printf("[%s]%d: Send U Station Meas Error, [i]\n",__FILE__, __LINE__, i);
	//}
}

/*Parse Complex Data B*/
static void parse_complex_data_b(Socket_Info_S	arrOpt[], unsigned char *pBuffer, const int length)
{
	unsigned char *pBuf = (unsigned char *)pBuffer;
	int s32Len = length;
	int i, s32SendLen=0, s32PassLen=0;
	int s32LikFraType, s32LikFraLen, s32VidFlag, s32VidLen;

#if 1
	/*FHB: Frame Cnt*/
	/* Reserve: Detect Whether Lost Valid Frame */
	if(((pBuf[5]<<8) | pBuf[4])==0){
		temp=0;
	}else{
		if((((pBuf[5]<<8) | pBuf[4])-temp) != 1){
			printf("[%s]%d:new****** last=%d , current=%d \n",__FILE__,__LINE__,temp, ((pBuf[5]<<8) | pBuf[4]));
			if(temp!=0){
				//exit(0);
			}
		}
		temp = (pBuf[5]<<8) | pBuf[4];
	}	
#endif

	/*1.Send L-Link Frame (L-Plane/L-Load/L-UpLikMeas/U-UpLikMeas)*/
	if((pBuf[POS_LB_FRAME_HEAD]==0x09)&&(pBuf[POS_LB_FRAME_HEAD+1]==0xD7)){
		s32LikFraType = pBuf[POS_LB_FRAME_TYPE] & 0x0F;
		s32LikFraLen = pBuf[POS_LB_FRAME_LEN];	
		if((s32LikFraType!=0)&&(s32LikFraLen<=MAX_DATA_LEN68)){
			for(i=0; i<L_FRAME_TYPE_NUM; i++){
				if((s32LikFraType>>i)==0x01){
					i+=NUM_L_PLANE;
					s32SendLen = send_udp_socket_data(&arrOpt[i], pBuf+LB_LINK_HEAD_LEN, s32LikFraLen);
					if(s32SendLen != s32LikFraLen)
						printf("[%s]%d: Send Down Link Frame Error, [%d]\n",__FILE__, __LINE__, i);
					break;
				}
			}
		}
	}
	s32PassLen += LB_LINK_LEN;

	/*2.Send Holder Report*/
	s32SendLen = send_udp_socket_data(&arrOpt[NUM_HOLDER], pBuf+s32PassLen, HOLDER_LEN);
	if(s32SendLen != HOLDER_LEN)
		printf("[%s]%d: Send Holder Error, [i]\n",__FILE__, __LINE__, NUM_HOLDER);
	s32PassLen += HOLDER_LEN;

	/*3.Send L Station Meas*/
	if((pBuf[s32PassLen]==0xEB)&&(pBuf[s32PassLen+1]==0x90)){
		s32SendLen = send_udp_socket_data(&arrOpt[NUM_L_STAT_MEAS], pBuf+s32PassLen, STATION_MEAS_LEN);
		if(s32SendLen != STATION_MEAS_LEN)
			printf("[%s]%d: Send L Station Meas Error, [i]\n",__FILE__, __LINE__, NUM_L_STAT_MEAS);
	}
	s32PassLen += STATION_MEAS_LEN;

	/*4.Send U-Link Frame (U-Plane/U-Load/U-UpLikMeas/U-UpLikMeas)*/
	if((pBuf[s32PassLen]==0x09)&&(pBuf[s32PassLen+1]==0xD7)){
		s32LikFraType = pBuf[s32PassLen+POS_U_FRAME_TYPE]&0x0F; 
		s32LikFraLen = pBuf[s32PassLen+POS_U_FRAME_LEN];
		if((s32LikFraType!=0)&&(s32LikFraLen<MAX_DATA_LEN68)){		
			for(i=0; i<U_FRAME_TYPE_NUM; i++){
				if((s32LikFraType>>i)==0x01){
					i += NUM_U_PLANE;
					s32SendLen = send_udp_socket_data(&arrOpt[i], pBuf+s32PassLen+U_LINK_HEAD_LEN, s32LikFraLen);
					if(s32SendLen != s32LikFraLen)
						printf("[%s]%d: Send Down Link Frame Error, [%d]\n",__FILE__, __LINE__, i);
					break;
				}
			}
		}
	}
	s32PassLen += U_LINK_LEN;

	/*5.Send U Station Meas*/
	//if((pBuf[s32PassLen]==0xEB)&&(pBuf[s32PassLen+1]==0x90)){
	//	s32SendLen = send_udp_socket_data(&arrOpt[NUM_U_STAT_MEAS], pBuf+s32PassLen, STATION_MEAS_LEN);
	//	if(s32SendLen != STATION_MEAS_LEN)
	//		printf("[%s]%d: Send U Station Meas Error, [%d]\n",__FILE__, __LINE__, NUM_U_STAT_MEAS);
	//}
}

/*Recv and Decode Frame*/
void *thread_recv_dec_frame(void *ptr)
{
	int		i, s32Ret;		
	int	    s32ReadFlag, retHeadPos=0, pickHeadFlag=0,s32FrameLen=0;
	int		s32ReadSize, s32RecvLen, s32SendLen, s32TempLen=0, s32ResultLen=0;

	unsigned char	  *pRecvBuf = NULL;
	unsigned char	  *pResultBuf = NULL;
	pRecvBuf	= 	malloc_mem(unsigned char, CONFIG_RECV_LEN);							
	pResultBuf	= 	malloc_mem(unsigned char, CONFIG_RESULT_LEN);
	unsigned char	pHeadBuf[FRAME_HEAD_SIZE] = FRAME_HEAD;
	memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

	PICK_FRAME_S	pickOpt;
	pickOpt.buffLen = 0;
	pickOpt.rearLen = 0;
	pickOpt.pFrontMatch = malloc_mem(unsigned char, MATCH_SIZE);
	pickOpt.pRearMatch = malloc_mem(unsigned char, MATCH_SIZE);

	/*1.Init Gpio and Emif Interface*/
	int gpioFd = open(DM368_DEV_GPIO, O_RDWR);
	if(gpioFd < 0){
		printf("[%s]%d: Open %s Error \n",__FILE__, __LINE__, DM368_DEV_GPIO);		
		exit(EXIT_FAILURE);
	}
	int emifFd = open(DM368_DEV_EMIF, O_RDWR);
	if(emifFd < 0){
		printf("[%s]%d: Open %s Error \n",__FILE__, __LINE__, DM368_DEV_EMIF);		
		exit(EXIT_FAILURE);
	}

	/*2.Init Socket*/
	Socket_Info_S	 outSocketOpt[OUTPUT_DATA_CNT];
	socket_init(outSocketOpt);

	/*Master Loop*/
	while(1){
		s32ResultLen = FRAME_HEAD_SIZE;
		memset(pResultBuf, 0, CONFIG_RESULT_LEN);
		memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

		/*3.Pick Frame*/
		do{
			do{
				/*3-1.Read emif and send comp*/	
				if(pickOpt.buffLen == 0){		
					/*A.Judge emif read size*/
#if 0
					s32ReadSize = detect_emif_read_size(gpioFd);
					if(s32ReadSize == DATA_RATE_ZERO){
						s32FrameLen = SIZE_COMPLEX_B;
					}else if(s32ReadSize == DATA_RATE_FIR){
						s32FrameLen = SIZE_COMPLEX_A;
					}else{
						printf("Never Happen \n");
						exit(EXIT_FAILURE);
					}
#else 	
					s32ReadSize=(4*1024);						
					s32FrameLen = SIZE_COMPLEX_A;		//2018.08.02

#endif

					/*B.Judge emif read flag*/
					do {
						s32ReadFlag = detect_emif_read_flag(gpioFd);
					} while(s32ReadFlag==0);
					s32ReadFlag = 0; 

					/*C.Read emif data*/	
					s32RecvLen = read(emifFd, pRecvBuf, s32ReadSize);
					pickOpt.pBuff		= 	pRecvBuf;
					pickOpt.buffLen	=	s32RecvLen; 

				}/*End_3-1*/

				/*3-2.Handle half frame head*/					
				if(pickOpt.rearLen != 0){
					memset(pickOpt.pFrontMatch, 0, MATCH_SIZE);
					memcpy(pickOpt.pFrontMatch, pickOpt.pRearMatch, pickOpt.rearLen);
					pickOpt.frontLen = pickOpt.rearLen;
					pickOpt.rearLen = 0;
					memset(pickOpt.pRearMatch, 0, MATCH_SIZE);
				}

				/*3-3.Pick frame head*/
				retHeadPos = pick_frame_head(&pickOpt, pHeadBuf, FRAME_HEAD_SIZE);
				if((pickHeadFlag == 0)&&(retHeadPos == -FRAME_HEAD_SIZE)){
					pickOpt.buffLen = 0;
				}

				/*3-4.Handle valid frame head*/
				if(pickHeadFlag != 0){
					/*A.Invalid frame head in current buf*/
					if(retHeadPos == -FRAME_HEAD_SIZE){
						if(pickOpt.frontLen != 0){					
							memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, pickOpt.frontLen);
							s32ResultLen = s32ResultLen + pickOpt.frontLen;
							pickOpt.frontLen = 0;
						}
						s32TempLen = pickOpt.buffLen - pickOpt.rearLen; 						 
						if((s32ResultLen + s32TempLen)>s32FrameLen){
							s32ResultLen = FRAME_HEAD_SIZE;
							pickHeadFlag = 0;
							memset(pResultBuf+FRAME_HEAD_SIZE, 0, s32FrameLen - FRAME_HEAD_SIZE);
						}else{
							memcpy(pResultBuf+s32ResultLen, pickOpt.pBuff, s32TempLen);
							s32ResultLen = s32ResultLen + s32TempLen;
						}
						pickOpt.buffLen = 0;
					}

					/*B.Valid frame head in last and current buf*/
					if((retHeadPos > -FRAME_HEAD_SIZE)&&(retHeadPos <= 0)){
						s32TempLen = pickOpt.frontLen + retHeadPos;
						if((s32ResultLen + s32TempLen) != s32FrameLen){
							s32ResultLen = FRAME_HEAD_SIZE;
							memset(pResultBuf+FRAME_HEAD_SIZE, 0, s32FrameLen - FRAME_HEAD_SIZE);
							pickOpt.frontLen = 0;//2018.04.20_add
						}else{
							if(s32TempLen != 0){
								memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, s32TempLen);
								s32ResultLen = s32ResultLen + s32TempLen;
							}
							pickOpt.frontLen = 0;
							pickHeadFlag = 2;
						}
					}

					/*C.Valid frame head in current buf*/
					if(retHeadPos > 0){
						s32TempLen = pickOpt.frontLen + retHeadPos;
						if((s32ResultLen + s32TempLen) != s32FrameLen){
							s32ResultLen = FRAME_HEAD_SIZE;
							memset(pResultBuf+FRAME_HEAD_SIZE, 0, s32FrameLen - FRAME_HEAD_SIZE);
						}else{
							if(pickOpt.frontLen != 0){
								memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, pickOpt.frontLen);
								s32ResultLen = s32ResultLen + pickOpt.frontLen;
								pickOpt.frontLen =0;
							}
							memcpy(pResultBuf+s32ResultLen, pickOpt.pBuff, retHeadPos);
							s32ResultLen = s32ResultLen + retHeadPos;
							pickHeadFlag = 2;
						}
					}
				}/*End_3-4*/
			}while(retHeadPos == -FRAME_HEAD_SIZE);

			/*3-5.Adjust pick data buf info*/
			pickOpt.pBuff = pickOpt.pBuff + retHeadPos + FRAME_HEAD_SIZE;
			pickOpt.buffLen = pickOpt.buffLen - retHeadPos - FRAME_HEAD_SIZE;
			switch(pickHeadFlag){
			case 0:
				pickHeadFlag++;
				break;
			case 1:
				break;
			case 2:
				pickHeadFlag--;
				break;
			default:
				printf("[%s]%d: Never Happen, pickHeadFlag=%d (%s)\n",__FILE__,__LINE__,pickHeadFlag,__func__);
				exit(EXIT_FAILURE);
			}		
		}while(s32ResultLen != s32FrameLen); 	

		/*4.Decode Frame*/
		if(s32FrameLen==SIZE_COMPLEX_A){
			parse_complex_data_a(outSocketOpt, pResultBuf, s32ResultLen);
		}else if(s32FrameLen==SIZE_COMPLEX_B){
			parse_complex_data_b(outSocketOpt, pResultBuf, s32ResultLen);
		}else{
			printf("Never Happen \n");
			exit(EXIT_FAILURE);
		}

	}/*while(1)*/
}

/*Entry: Main*/
int main(int argc,char *argv[])
{
	int i, s32Ret = 0;
	int s32SendSize = 0;
	pthread_t	pthreadRecvDecFrame;
	pthread_t	pthreadVideo[VIDEO_NUM];

	/*0.Ignore SIGPIPE Signal*/
	struct sigaction newAct;
	newAct.sa_handler = SIG_IGN;
	if(sigaction(SIGPIPE, &newAct, NULL) != 0){
		printf("[%s]%d: ignore sigpipe error \n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	/*1.Alloc video memory*/
	for(i=0; i<VIDEO_NUM; i++){
		pVideoBuf[i]	= malloc_mem(unsigned char, CONFIG_VIDEO_LEN);
	}

	/*2.Setup the mutexes*/
	for(i=0; i<VIDEO_NUM; i++){
		pthread_mutex_init(&videoMut[i], NULL);
	}

	/*3.Setup the conditions*/
	for(i=0; i<VIDEO_NUM; i++){
		pthread_cond_init(&videoCond[i], NULL);
	}

    /*4.Create a thread to handle original composite data*/
	s32Ret = pthread_create(&pthreadRecvDecFrame, NULL, thread_recv_dec_frame, NULL);
	if(s32Ret != 0){
		printf("[%s]%d: Create pthreadRecvDecFrame Error\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	/*5.Create six thread to setup video rtsp server*/
	s32Ret = pthread_create(&pthreadVideo[0], NULL, thread_pick_zero_video, NULL);
	if(s32Ret != 0){
		printf("[%s]%d: Create pthreadVideo[0] Error\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
	s32Ret = pthread_create(&pthreadVideo[1], NULL, thread_pick_first_video, NULL);
	if(s32Ret != 0){
		printf("[%s]%d: Create pthreadVideo[1] Error\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
	s32Ret = pthread_create(&pthreadVideo[2], NULL, thread_pick_second_video, NULL);
	if(s32Ret != 0){
		printf("[%s]%d: Create pthreadVideo[2] Error\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	printf("[%s] begin ... \n", __FILE__);
	/*6.Wait thread exit*/
	pthread_join(pthreadRecvDecFrame, NULL);
	for(i=0; i<VIDEO_NUM; i++){
		pthread_join(pthreadVideo[i], NULL);
	}

	return 0;
}