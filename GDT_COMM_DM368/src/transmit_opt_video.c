/******************************************************************************
 	Program         :	GDT_COMM_DM368 of Transmit Video (RTSP)  (for DM368)
 	Author           :	ZhangJ
	Modification	:	2018-05 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "system_config.h"
#include "transmit_interface.h"

#define FRAME_HEAD						{0x00, 0x00, 0x00, 0x01}
#define FRAME_HEAD_SIZE				4
#define MATCH_SIZE						((FRAME_HEAD_SIZE-1)*2)

#define FRAME_TYPE_CNT	10
unsigned char frameTypeArray[FRAME_TYPE_CNT]={0x40, 0x42, 0x44, 0x4E, 0x26, 0x02, 0x67, 0x68, 0x65, 0x61};

/*Task Video*/
void *thread_pick_zero_video(void *ptr)
{	
	int		i, s32Ret, s32WriteSize;
	int		retHeadPos=0, pickHeadFlag=0;
	int		s32RecvLen, s32TempLen=0, s32ResultLen=0;
	unsigned char	  *pRecvBuf	= NULL;
	unsigned char	  *pResultBuf	= NULL;
	pRecvBuf	= 	malloc_mem(unsigned char, CONFIG_RECV_LEN);							
	pResultBuf	= 	malloc_mem(unsigned char, CONFIG_VIDEO_LEN);
	unsigned char	pHeadBuf[FRAME_HEAD_SIZE] = FRAME_HEAD;
	memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

	PICK_FRAME_S	pickOpt;
	pickOpt.buffLen = 0;
	pickOpt.rearLen = 0;
	pickOpt.pFrontMatch = malloc_mem(unsigned char, MATCH_SIZE);
	pickOpt.pRearMatch = malloc_mem(unsigned char, MATCH_SIZE);

	/*1.Init Fifo*/
	FILE_OPT_S fileOpt;
	fileOpt.pFileName	= V0_FIFO_NAME;
	fileOpt.optMode		= O_WRONLY;
	if(init_file(&fileOpt) !=0){
		printf("[%s]%d: Open %s Error (%s)\n", __FILE__, __LINE__, fileOpt.pFileName, __func__);
		exit(EXIT_FAILURE);
	}else{
		pthread_mutex_lock(&videoMut[0]);
		writeFlag[0] = VALID;
		pthread_mutex_unlock(&videoMut[0]);
	}

	while(1){

		s32ResultLen = FRAME_HEAD_SIZE;
		memset(pResultBuf, 0, CONFIG_VIDEO_LEN);
		memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

		/*2.Pick Frame*/
		while(1){
			do{ 
				/*2-1.Wait video data*/	
				if(pickOpt.buffLen == 0){
					pthread_mutex_lock(&videoMut[0]);
					while(s32VideoLen[0]==0){
						pthread_cond_wait(&videoCond[0], &videoMut[0]);
					}
					memcpy(pRecvBuf, pVideoBuf[0], s32VideoLen[0]);
					memset(pVideoBuf[0], 0, s32VideoLen[0]);
					s32RecvLen = s32VideoLen[0];
					s32VideoLen[0] = 0;	
					pickOpt.pBuff		= 	pRecvBuf;
					pickOpt.buffLen	=	s32RecvLen;	
					pthread_mutex_unlock(&videoMut[0]);
				} 		

				/*2-2.Pick video frame*/
				if(pickOpt.rearLen != 0){
					memset(pickOpt.pFrontMatch, 0, MATCH_SIZE);
					memcpy(pickOpt.pFrontMatch, pickOpt.pRearMatch, pickOpt.rearLen); 
					pickOpt.frontLen = pickOpt.rearLen;
					pickOpt.rearLen = 0;
					memset(pickOpt.pRearMatch, 0, MATCH_SIZE);
				}else{
					memset(pickOpt.pFrontMatch, 0, MATCH_SIZE);
					memset(pickOpt.pRearMatch, 0, MATCH_SIZE);
					pickOpt.frontLen = 0;
				}

				retHeadPos = pick_frame_head(&pickOpt, pHeadBuf, FRAME_HEAD_SIZE);
				if((pickHeadFlag == 0)&&(retHeadPos == -FRAME_HEAD_SIZE)){
					pickOpt.buffLen = 0;
				}

				if(pickHeadFlag != 0){
					/* Invalid Frame Head in Current Buf */
					if(retHeadPos == -FRAME_HEAD_SIZE){
						if(pickOpt.frontLen != 0){
							memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, pickOpt.frontLen);
							s32ResultLen = s32ResultLen + pickOpt.frontLen;
							pickOpt.frontLen = 0;
						}
						s32TempLen = pickOpt.buffLen - pickOpt.rearLen; 						 
						memcpy(pResultBuf+s32ResultLen, pickOpt.pBuff, s32TempLen);
						s32ResultLen = s32ResultLen + s32TempLen;
						pickOpt.buffLen = 0;
					}

					/* Valid Frame Head in (Last & Current Buf) */
					if((retHeadPos > -FRAME_HEAD_SIZE)&&(retHeadPos <= 0)){
						s32TempLen = pickOpt.frontLen + retHeadPos;
						if(s32TempLen != 0){
							memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, s32TempLen);
							s32ResultLen = s32ResultLen + s32TempLen;
						}
						pickOpt.frontLen = 0;
						pickHeadFlag = 2;
					}

					/* Valid Frame Head in (Current Buf) */
					if(retHeadPos > 0){
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
			}while(retHeadPos == -FRAME_HEAD_SIZE); 

			/* Flush pBuff and buffLen */
			pickOpt.pBuff = pickOpt.pBuff + retHeadPos + FRAME_HEAD_SIZE;
			pickOpt.buffLen = pickOpt.buffLen - retHeadPos - FRAME_HEAD_SIZE;

			if(pickHeadFlag==0){
				pickHeadFlag++;
			}else if(pickHeadFlag==1){
				/* go on */
			}else if(pickHeadFlag==2){
				pickHeadFlag--;
				break;
			}else{
				printf("[%s]%d: Never Happen, pickHeadFlag=%d (%s)\n",__FILE__,__LINE__,pickHeadFlag,__func__);
				exit(EXIT_FAILURE);
			}
		}

		/*3.Handle Valid Video Frame*/
		for(i=0; i<FRAME_TYPE_CNT; i++){
			if(pResultBuf[4]==frameTypeArray[i]){
				s32WriteSize = write_file(&fileOpt, pResultBuf, s32ResultLen);
				if(s32WriteSize < 0){
					pthread_mutex_lock(&videoMut[0]);
					writeFlag[0] = INVALID;
					s32VideoLen[0] = 0;
					pthread_mutex_unlock(&videoMut[0]);
					release_file(&fileOpt);
					if(init_file(&fileOpt) !=0){
						printf("[%s]%d: Open %s Error (%s)\n", __FILE__, __LINE__, fileOpt.pFileName, __func__);
						exit(EXIT_FAILURE);
					}else{
						pthread_mutex_lock(&videoMut[0]);
						writeFlag[0] = VALID;
						pthread_mutex_unlock(&videoMut[0]);
					}
				}else{
					//printf("[Video_0] s32WriteSize = %d \n", s32WriteSize);
				}
				break;
			}
		}
	}/*while(1)*/
}
void *thread_pick_first_video(void *ptr)
{	
	int		i, s32Ret, s32WriteSize;
	int		retHeadPos=0, pickHeadFlag=0;
	int		s32RecvLen, s32TempLen=0, s32ResultLen=0;
	unsigned char	  *pRecvBuf	= NULL;
	unsigned char	  *pResultBuf	= NULL;
	pRecvBuf	= 	malloc_mem(unsigned char, CONFIG_RECV_LEN);							
	pResultBuf	= 	malloc_mem(unsigned char, CONFIG_VIDEO_LEN);
	unsigned char	pHeadBuf[FRAME_HEAD_SIZE] = FRAME_HEAD;
	memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

	PICK_FRAME_S	pickOpt;
	pickOpt.buffLen = 0;
	pickOpt.rearLen = 0;
	pickOpt.pFrontMatch = malloc_mem(unsigned char, MATCH_SIZE);
	pickOpt.pRearMatch = malloc_mem(unsigned char, MATCH_SIZE);

	/*1.Init Fifo*/
	FILE_OPT_S fileOpt;
	fileOpt.pFileName	= V1_FIFO_NAME;
	fileOpt.optMode		= O_WRONLY;
	if(init_file(&fileOpt) !=0){
		printf("[%s]%d: Open %s Error (%s)\n", __FILE__, __LINE__, fileOpt.pFileName, __func__);
		exit(EXIT_FAILURE);
	}else{
		pthread_mutex_lock(&videoMut[1]);
		writeFlag[1] = VALID;
		pthread_mutex_unlock(&videoMut[1]);
	}

	while(1){

		s32ResultLen = FRAME_HEAD_SIZE;
		memset(pResultBuf, 0, CONFIG_VIDEO_LEN);
		memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

		/*2.Pick Frame*/
		while(1){
			do{ 
				/*2-1.Wait video data*/	
				if(pickOpt.buffLen == 0){
					pthread_mutex_lock(&videoMut[1]);
					while(s32VideoLen[1]==0){
						pthread_cond_wait(&videoCond[1], &videoMut[1]);
					}
					memcpy(pRecvBuf, pVideoBuf[1], s32VideoLen[1]);
					memset(pVideoBuf[1], 0, s32VideoLen[1]);
					s32RecvLen = s32VideoLen[1];
					s32VideoLen[1] = 0;	
					pickOpt.pBuff		= 	pRecvBuf;
					pickOpt.buffLen	=	s32RecvLen;	
					pthread_mutex_unlock(&videoMut[1]);
				} 		

				/*2-2.Pick video frame*/
				if(pickOpt.rearLen != 0){
					memset(pickOpt.pFrontMatch, 0, MATCH_SIZE);
					memcpy(pickOpt.pFrontMatch, pickOpt.pRearMatch, pickOpt.rearLen); 
					pickOpt.frontLen = pickOpt.rearLen;
					pickOpt.rearLen = 0;
					memset(pickOpt.pRearMatch, 0, MATCH_SIZE);
				}else{
					memset(pickOpt.pFrontMatch, 0, MATCH_SIZE);
					memset(pickOpt.pRearMatch, 0, MATCH_SIZE);
					pickOpt.frontLen = 0;
				}

				retHeadPos = pick_frame_head(&pickOpt, pHeadBuf, FRAME_HEAD_SIZE);
				if((pickHeadFlag == 0)&&(retHeadPos == -FRAME_HEAD_SIZE)){
					pickOpt.buffLen = 0;
				}

				if(pickHeadFlag != 0){
					/* Invalid Frame Head in Current Buf */
					if(retHeadPos == -FRAME_HEAD_SIZE){
						if(pickOpt.frontLen != 0){
							memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, pickOpt.frontLen);
							s32ResultLen = s32ResultLen + pickOpt.frontLen;
							pickOpt.frontLen = 0;
						}
						s32TempLen = pickOpt.buffLen - pickOpt.rearLen; 						 
						memcpy(pResultBuf+s32ResultLen, pickOpt.pBuff, s32TempLen);
						s32ResultLen = s32ResultLen + s32TempLen;
						pickOpt.buffLen = 0;
					}

					/* Valid Frame Head in (Last & Current Buf) */
					if((retHeadPos > -FRAME_HEAD_SIZE)&&(retHeadPos <= 0)){
						s32TempLen = pickOpt.frontLen + retHeadPos;
						if(s32TempLen != 0){
							memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, s32TempLen);
							s32ResultLen = s32ResultLen + s32TempLen;
						}
						pickOpt.frontLen = 0;
						pickHeadFlag = 2;
					}

					/* Valid Frame Head in (Current Buf) */
					if(retHeadPos > 0){
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
			}while(retHeadPos == -FRAME_HEAD_SIZE); 

			/* Flush pBuff and buffLen */
			pickOpt.pBuff = pickOpt.pBuff + retHeadPos + FRAME_HEAD_SIZE;
			pickOpt.buffLen = pickOpt.buffLen - retHeadPos - FRAME_HEAD_SIZE;

			if(pickHeadFlag==0){
				pickHeadFlag++;
			}else if(pickHeadFlag==1){
				/* go on */
			}else if(pickHeadFlag==2){
				pickHeadFlag--;
				break;
			}else{
				printf("[%s]%d: Never Happen, pickHeadFlag=%d (%s)\n",__FILE__,__LINE__,pickHeadFlag,__func__);
				exit(EXIT_FAILURE);
			}
		}

		/*3.Handle Valid Video Frame*/
		for(i=0; i<FRAME_TYPE_CNT; i++){
			if(pResultBuf[4]==frameTypeArray[i]){
				s32WriteSize = write_file(&fileOpt, pResultBuf, s32ResultLen);
				if(s32WriteSize < 0){
					pthread_mutex_lock(&videoMut[1]);
					writeFlag[1] = INVALID;
					s32VideoLen[1] = 0;
					pthread_mutex_unlock(&videoMut[1]);
					release_file(&fileOpt);
					if(init_file(&fileOpt) !=0){
						printf("[%s]%d: Open %s Error (%s)\n", __FILE__, __LINE__, fileOpt.pFileName, __func__);
						exit(EXIT_FAILURE);
					}else{
						pthread_mutex_lock(&videoMut[1]);
						writeFlag[1] = VALID;
						pthread_mutex_unlock(&videoMut[1]);
					}
				}else{
					//printf("[Video_1] s32WriteSize = %d \n", s32WriteSize);
				}
				break;
			}
		}
	}/*while(1)*/
}
void *thread_pick_second_video(void *ptr)
{	
	int		i, s32Ret, s32WriteSize;
	int		retHeadPos=0, pickHeadFlag=0;
	int		s32RecvLen, s32TempLen=0, s32ResultLen=0;
	unsigned char	  *pRecvBuf	= NULL;
	unsigned char	  *pResultBuf	= NULL;
	pRecvBuf	= 	malloc_mem(unsigned char, CONFIG_RECV_LEN);							
	pResultBuf	= 	malloc_mem(unsigned char, CONFIG_VIDEO_LEN);
	unsigned char	pHeadBuf[FRAME_HEAD_SIZE] = FRAME_HEAD;
	memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

	PICK_FRAME_S	pickOpt;
	pickOpt.buffLen = 0;
	pickOpt.rearLen = 0;
	pickOpt.pFrontMatch = malloc_mem(unsigned char, MATCH_SIZE);
	pickOpt.pRearMatch = malloc_mem(unsigned char, MATCH_SIZE);

	/*1.Init Fifo*/
	FILE_OPT_S fileOpt;
	fileOpt.pFileName	=V2_FIFO_NAME;
	fileOpt.optMode		= O_WRONLY;
	if(init_file(&fileOpt) !=0){
		printf("[%s]%d: Open %s Error (%s)\n", __FILE__, __LINE__, fileOpt.pFileName, __func__);
		exit(EXIT_FAILURE);
	}else{
		pthread_mutex_lock(&videoMut[2]);
		writeFlag[2] = VALID;
		pthread_mutex_unlock(&videoMut[2]);
	}

	while(1){

		s32ResultLen = FRAME_HEAD_SIZE;
		memset(pResultBuf, 0, CONFIG_VIDEO_LEN);
		memcpy(pResultBuf, pHeadBuf, FRAME_HEAD_SIZE);

		/*2.Pick Frame*/
		while(1){
			do{ 
				/*2-1.Wait video data*/	
				if(pickOpt.buffLen == 0){
					pthread_mutex_lock(&videoMut[2]);
					while(s32VideoLen[2]==0){
						pthread_cond_wait(&videoCond[2], &videoMut[2]);
					}
					memcpy(pRecvBuf, pVideoBuf[2], s32VideoLen[2]);
					memset(pVideoBuf[2], 0, s32VideoLen[2]);
					s32RecvLen = s32VideoLen[2];
					s32VideoLen[2] = 0;	
					pickOpt.pBuff		= 	pRecvBuf;
					pickOpt.buffLen	=	s32RecvLen;	
					pthread_mutex_unlock(&videoMut[2]);
				} 		

				/*2-2.Pick video frame*/
				if(pickOpt.rearLen != 0){
					memset(pickOpt.pFrontMatch, 0, MATCH_SIZE);
					memcpy(pickOpt.pFrontMatch, pickOpt.pRearMatch, pickOpt.rearLen); 
					pickOpt.frontLen = pickOpt.rearLen;
					pickOpt.rearLen = 0;
					memset(pickOpt.pRearMatch, 0, MATCH_SIZE);
				}else{
					memset(pickOpt.pFrontMatch, 0, MATCH_SIZE);
					memset(pickOpt.pRearMatch, 0, MATCH_SIZE);
					pickOpt.frontLen = 0;
				}

				retHeadPos = pick_frame_head(&pickOpt, pHeadBuf, FRAME_HEAD_SIZE);
				if((pickHeadFlag == 0)&&(retHeadPos == -FRAME_HEAD_SIZE)){
					pickOpt.buffLen = 0;
				}

				if(pickHeadFlag != 0){
					/* Invalid Frame Head in Current Buf */
					if(retHeadPos == -FRAME_HEAD_SIZE){
						if(pickOpt.frontLen != 0){
							memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, pickOpt.frontLen);
							s32ResultLen = s32ResultLen + pickOpt.frontLen;
							pickOpt.frontLen = 0;
						}
						s32TempLen = pickOpt.buffLen - pickOpt.rearLen; 						 
						memcpy(pResultBuf+s32ResultLen, pickOpt.pBuff, s32TempLen);
						s32ResultLen = s32ResultLen + s32TempLen;
						pickOpt.buffLen = 0;
					}

					/* Valid Frame Head in (Last & Current Buf) */
					if((retHeadPos > -FRAME_HEAD_SIZE)&&(retHeadPos <= 0)){
						s32TempLen = pickOpt.frontLen + retHeadPos;
						if(s32TempLen != 0){
							memcpy(pResultBuf+s32ResultLen, pickOpt.pFrontMatch, s32TempLen);
							s32ResultLen = s32ResultLen + s32TempLen;
						}
						pickOpt.frontLen = 0;
						pickHeadFlag = 2;
					}

					/* Valid Frame Head in (Current Buf) */
					if(retHeadPos > 0){
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
			}while(retHeadPos == -FRAME_HEAD_SIZE); 

			/* Flush pBuff and buffLen */
			pickOpt.pBuff = pickOpt.pBuff + retHeadPos + FRAME_HEAD_SIZE;
			pickOpt.buffLen = pickOpt.buffLen - retHeadPos - FRAME_HEAD_SIZE;

			if(pickHeadFlag==0){
				pickHeadFlag++;
			}else if(pickHeadFlag==1){
				/* go on */
			}else if(pickHeadFlag==2){
				pickHeadFlag--;
				break;
			}else{
				printf("[%s]%d: Never Happen, pickHeadFlag=%d (%s)\n",__FILE__,__LINE__,pickHeadFlag,__func__);
				exit(EXIT_FAILURE);
			}
		}

		/*3.Handle Valid Video Frame*/
		for(i=0; i<FRAME_TYPE_CNT; i++){
			if(pResultBuf[4]==frameTypeArray[i]){
				s32WriteSize = write_file(&fileOpt, pResultBuf, s32ResultLen);
				if(s32WriteSize < 0){
					pthread_mutex_lock(&videoMut[2]);
					writeFlag[2] = INVALID;
					s32VideoLen[2] = 0;
					pthread_mutex_unlock(&videoMut[2]);
					release_file(&fileOpt);
					if(init_file(&fileOpt) !=0){
						printf("[%s]%d: Open %s Error (%s)\n", __FILE__, __LINE__, fileOpt.pFileName, __func__);
						exit(EXIT_FAILURE);
					}else{
						pthread_mutex_lock(&videoMut[2]);
						writeFlag[2] = VALID;
						pthread_mutex_unlock(&videoMut[2]);
					}
				}else{
					//printf("[Video_2] s32WriteSize = %d \n", s32WriteSize);
				}
				break;
			}
		}
	}/*while(1)*/
}
