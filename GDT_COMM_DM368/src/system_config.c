/******************************************************************************
 	Program         :	GDT_COMM_DM368 of System Config 
 	Author           :	ZhangJ
	Modification	:	2018-05 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "system_config.h"

/*Video Info (Global Define)*/
unsigned char *pVideoBuf[VIDEO_NUM];
int writeFlag[VIDEO_NUM]={INVALID, INVALID, INVALID};
int s32VideoLen[VIDEO_NUM];
pthread_mutex_t	videoMut[VIDEO_NUM];
pthread_cond_t videoCond[VIDEO_NUM];

/* Pick Frame Head */
int pick_frame_head(PICK_FRAME_S *opt, unsigned char *frameHead, int frameHeadLen)
 {
 	int i=0, j=0;
 	int validFlag = 0;
 
 	/* 1.Pick pFrontMatch, frontLen_(0, frameHeadLen), must make sure ( buffLen > frameHeadLen-1) */
  	if((opt->frontLen > 0)&&(opt->frontLen < frameHeadLen)){
  		for(i=0; i<frameHeadLen-1; i++){
  			opt->pFrontMatch[opt->frontLen+i] = opt->pBuff[i];
  		}
  
  		/* Match all frame head */
  		for(i=0; i<opt->frontLen; i++){ 			
  			for(j=0; j<frameHeadLen; j++){
  				if(opt->pFrontMatch[i+j] != frameHead[j]){
  					validFlag = 1;   
  					break;
  				}
  			}
			if(0 != validFlag)	{
  				validFlag = 0;	
			}else{
				opt->rearLen =0; //2017.10.13
				return (i - opt->frontLen); 
			}
  		}		
  	}
 
	/* 2.Pick pBuff, buffLen_[0, frameHeadLen) */
	if (opt->buffLen < frameHeadLen){
		/* Match half frame head */
		for(i=0; i<opt->buffLen; i++){
			for(j=0; j<opt->buffLen-i; j++){
				if(opt->pBuff[i+j] != frameHead[j]){
					validFlag = 1;
					break;
				}
			}
			if(0 != validFlag){
				validFlag = 0;
			}else{	
				opt->rearLen = opt->buffLen - i;
				memcpy(opt->pRearMatch, opt->pBuff+i, opt->rearLen);
				return -frameHeadLen;
			}
		}
	}	

 	/* 3.Pick pBuff, buffLen_[frameHeadLen, buffLen] */
 	if (opt->buffLen >= frameHeadLen){

 		/* Match all frame head */
 		for(i=0; i<(opt->buffLen-(frameHeadLen-1)); i++){
 			for(j=0; j<frameHeadLen; j++){
 				if(opt->pBuff[i+j] != frameHead[j]){
 					validFlag = 1;
 					break;
 				}
 			}
			if(0 != validFlag){
 				validFlag = 0;
			}else{
				opt->rearLen =0; //2017.10.13
 				return i;
			}
 		}

		/* Match half frame head */
		for(; i<opt->buffLen; i++){
			for(j=0; j<opt->buffLen-i; j++){
				if(opt->pBuff[i+j] != frameHead[j]){
					validFlag = 1;
					break;
				}
			}
			if(0 != validFlag){
				validFlag = 0;
			}else{	
				opt->rearLen = opt->buffLen - i;
				memcpy(opt->pRearMatch, opt->pBuff+i, opt->rearLen);
				return -frameHeadLen;
			}
		}
 	}

 	opt->rearLen = 0;
 	return -frameHeadLen;
 }

/* Detect Emif Read Flag*/
int detect_emif_read_flag(int fd)
{
	int flag;
	ioctl(fd, EMIF_READ_FLAG, &flag);
	return flag;
}

/* Detect Emif Read Unit Size*/
int detect_emif_read_size(int fd)
{
	int firFlag=0, secFlag=0;
	int s32ReadSize=0;
	ioctl(fd, EMIF_READ_SIZE_FIR_FLAG, &firFlag);
	ioctl(fd, EMIF_READ_SIZE_SEC_FLAG, &secFlag);
	//printf("firFlag=%d, secFlag=%d \n",firFlag,secFlag);

	if((firFlag==0)&&(secFlag==0)){
		return DATA_RATE_ZERO;
	}else if((firFlag==0)&&(secFlag!=0)){
		return DATA_RATE_FIR;
	}else if((firFlag!=0)&&(secFlag==0)){
		return DATA_RATE_SEC;
	}else if((firFlag!=0)&&(secFlag!=0)){
		return DATA_RATE_THR;
	}else{
		printf("[%s]%d: Never Happen ! \n", __func__, __LINE__);
		exit(EXIT_FAILURE);
	}							
}
