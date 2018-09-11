/******************************************************************************
 	Program         :	GDT_COMM_DM368 of Transmit Control Command  (for DM368)
 	Author           :	ZhangJ
	Modification	:	2018-01 Created
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

/*Remote Control Info*/

#define RECV_BUF_SIZE					(64*1024)

/*Contel Frame Info*/
#define FRAME_LEN							72
#define FRAME_INFO_LEN				4
#define DATA_MAX_LEN					68
#define FRAME_HEAD_FIR				0x09
#define FRAME_HEAD_SEC				0xD7
#define POS_DATA_TYPE					2
#define POS_DATA_LEN					3

/*Priority Info*/
#define PRIO_BASE_VAL					50
#define PRIO_STEP							2

typedef enum{
	TYPE_L_PLANE		= 0x01,
	TYPE_U_PLANE		= 0x02,
	TYPE_L_LOAD			= 0x04,
	TYPE_U_LOAD			= 0x08,
	TYPE_HOLDER			= 0x10,
	TYPE_LINK				= 0x20,
	TYPE_PARAM_REG	= 0x40,
}FRAME_TYPE_E;

pthread_mutex_t	spiMut;


/*Temp Func (test)*///2018.07.24
//Socket_Info_S testSockOpt;
//static void test_socket_init(Socket_Info_S *pTestSockOpt)
//{
//	int  s32Ret;
//	Socket_Info_S	 *pSockOpt = pTestSockOpt;
//	pSockOpt->selCastFlag		=	MULTICAST_SEND;
//	pSockOpt->bindLocalFlag		=	BIND_LOCAL_IP;
//	pSockOpt->pLocalAddr			=	LOCAL_ETH0_IP;
//	pSockOpt->localPort			=  30000;
//	pSockOpt->pMcastAddr			=	"224.0.1.2";
//	pSockOpt->mcastPort			=  9090; 
//	s32Ret = init_udp_socket(pSockOpt);
//	if(s32Ret < 0){
//		printf("[%s]%d: Init Socket Error\n",__FILE__, __LINE__);
//		exit(EXIT_FAILURE);
//	}
//}

/*0.Transmit L Plane Cmd*/
void *thread_L_plane_cmd(void *ptr)
{
	int fd = *(int *)ptr;
	int i, s32Ret;
	int s32RecvLen=0, s32SendLen=0, s32DataLen=0, s32DataType=TYPE_L_PLANE;
	int s32CntFlag=0, s32CntTime=0, s32DataBlockLen=DATA_MAX_LEN;
	unsigned char pBuf[RECV_BUF_SIZE];
	unsigned char pSendBuf[FRAME_LEN]={FRAME_HEAD_FIR, FRAME_HEAD_SEC};

	/* 1.Socket for Recv Control Cmd */
	Socket_Info_S inSocketOpt;
	inSocketOpt.selCastFlag		=	MULTICAST_RECV;
	inSocketOpt.bindLocalFlag	=	BIND_LOCAL_IP;
	inSocketOpt.pLocalAddr		=	LOCAL_ETH0_IP;
	inSocketOpt.localPort			=	CTRL_L_PLANE_PORT;
	inSocketOpt.pMcastAddr		=	CTRL_CMD_IP;
	s32Ret = init_udp_socket(&inSocketOpt);
	if(s32Ret < 0){
		printf("[%s]%d: Init Socket Error\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		s32RecvLen = recv_udp_socket_data(&inSocketOpt, pBuf, RECV_BUF_SIZE);
		if(s32RecvLen<0){
			printf("[%s]%d: Recv Data Error(%d)\n", __FILE__, __LINE__, s32RecvLen);
		}else{
			//printf("Recv_L_plane_cmd = %d\n", s32RecvLen);
			s32CntFlag = (s32RecvLen/(s32DataBlockLen))+1;
			if((s32RecvLen%(s32DataBlockLen))==0){
				s32CntFlag--;
			}			
			s32CntTime = s32CntFlag;

			for(i=0; i<s32CntFlag; i++){
				if(s32CntTime!=1){
					s32DataLen = s32DataBlockLen;
					s32CntTime--;
				}else{
					s32DataLen = s32RecvLen-(i*s32DataBlockLen);
				}

				pSendBuf[POS_DATA_TYPE] = s32DataType;
				pSendBuf[POS_DATA_LEN] = s32DataLen;
				memset(pSendBuf+FRAME_INFO_LEN, 0, s32DataBlockLen);
				memcpy(pSendBuf+FRAME_INFO_LEN, pBuf+(i*s32DataBlockLen), s32DataLen);
				pthread_mutex_lock(&spiMut);
				s32SendLen = write(fd, pSendBuf, FRAME_LEN);
				if(s32SendLen != FRAME_LEN)
					printf("[%s]%d: Send ctrlCmd Error(%d)\n", __FILE__, __LINE__, s32SendLen);	
				//send_udp_socket_data(&testSockOpt, pSendBuf, FRAME_LEN);//2018.07.24
				pthread_mutex_unlock(&spiMut);
			}
		}
	} /*while(1)*/
}

/*1.Transmit U Plane Cmd*/
void *thread_U_plane_cmd(void *ptr)
{
	int fd = *(int *)ptr;
	int i, s32Ret;
	int s32RecvLen=0, s32SendLen=0, s32DataLen=0, s32DataType=TYPE_U_PLANE;
	int s32CntFlag=0, s32CntTime=0, s32DataBlockLen=DATA_MAX_LEN;
	unsigned char pBuf[RECV_BUF_SIZE];
	unsigned char pSendBuf[FRAME_LEN]={FRAME_HEAD_FIR, FRAME_HEAD_SEC};

	/* 1.Socket for Recv Control Cmd */
	Socket_Info_S inSocketOpt;
	inSocketOpt.selCastFlag		=	MULTICAST_RECV;
	inSocketOpt.bindLocalFlag	=	BIND_LOCAL_IP;
	inSocketOpt.pLocalAddr		=	LOCAL_ETH0_IP;
	inSocketOpt.localPort			=	CTRL_U_PLANE_PORT;
	inSocketOpt.pMcastAddr		=	CTRL_CMD_IP;
	s32Ret = init_udp_socket(&inSocketOpt);
	if(s32Ret < 0){
		printf("[%s]%d: Init Socket Error\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		s32RecvLen = recv_udp_socket_data(&inSocketOpt, pBuf, RECV_BUF_SIZE);
		if(s32RecvLen<0){
			printf("[%s]%d: Recv Data Error(%d)\n", __FILE__, __LINE__, s32RecvLen);
		}else{
			//printf("Recv_U_plane_cmd = %d\n",s32RecvLen);
			s32CntFlag = (s32RecvLen/(s32DataBlockLen))+1;
			if((s32RecvLen%(s32DataBlockLen))==0){
				s32CntFlag--;
			}			
			s32CntTime = s32CntFlag;

			for(i=0; i<s32CntFlag; i++){
				if(s32CntTime!=1){
					s32DataLen = s32DataBlockLen;
					s32CntTime--;
				}else{
					s32DataLen = s32RecvLen-(i*s32DataBlockLen);
				}

				pSendBuf[POS_DATA_TYPE] = s32DataType;
				pSendBuf[POS_DATA_LEN] = s32DataLen;
				memset(pSendBuf+FRAME_INFO_LEN, 0, s32DataBlockLen);
				memcpy(pSendBuf+FRAME_INFO_LEN, pBuf+(i*s32DataBlockLen), s32DataLen);
				pthread_mutex_lock(&spiMut);
				s32SendLen = write(fd, pSendBuf, FRAME_LEN);
				if(s32SendLen != FRAME_LEN)
					printf("[%s]%d: Send ctrlCmd Error(%d)\n", __FILE__, __LINE__, s32SendLen);	
				//send_udp_socket_data(&testSockOpt, pSendBuf, FRAME_LEN);//2018.07.24
				pthread_mutex_unlock(&spiMut);
			}
		}
	} /*while(1)*/
}

/*2.Transmit L Load Cmd*/
void *thread_L_load_cmd(void *ptr)
{
	int fd = *(int *)ptr;
	int i, s32Ret;
	int s32RecvLen=0, s32SendLen=0, s32DataLen=0, s32DataType=TYPE_L_LOAD;
	int s32CntFlag=0, s32CntTime=0, s32DataBlockLen=DATA_MAX_LEN;
	unsigned char pBuf[RECV_BUF_SIZE];
	unsigned char pSendBuf[FRAME_LEN]={FRAME_HEAD_FIR, FRAME_HEAD_SEC};

	/* 1.Socket for Recv Control Cmd */
	Socket_Info_S inSocketOpt;
	inSocketOpt.selCastFlag		=	MULTICAST_RECV;
	inSocketOpt.bindLocalFlag	=	BIND_LOCAL_IP;
	inSocketOpt.pLocalAddr		=	LOCAL_ETH0_IP;
	inSocketOpt.localPort			=	CTRL_L_LOAD_PORT;
	inSocketOpt.pMcastAddr		=	CTRL_CMD_IP;
	s32Ret = init_udp_socket(&inSocketOpt);
	if(s32Ret < 0){
		printf("[%s]%d: Init Socket Error\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		s32RecvLen = recv_udp_socket_data(&inSocketOpt, pBuf, RECV_BUF_SIZE);
		if(s32RecvLen<0){
			printf("[%s]%d: Recv Data Error(%d)\n", __FILE__, __LINE__, s32RecvLen);
		}else{
			//printf("Recv_L_load_cmd = %d \n",s32RecvLen);
			s32CntFlag = (s32RecvLen/(s32DataBlockLen))+1;
			if((s32RecvLen%(s32DataBlockLen))==0){
				s32CntFlag--;
			}			
			s32CntTime = s32CntFlag;

			for(i=0; i<s32CntFlag; i++){
				if(s32CntTime!=1){
					s32DataLen = s32DataBlockLen;
					s32CntTime--;
				}else{
					s32DataLen = s32RecvLen-(i*s32DataBlockLen);
				}

				pSendBuf[POS_DATA_TYPE] = s32DataType;
				pSendBuf[POS_DATA_LEN] = s32DataLen;
				memset(pSendBuf+FRAME_INFO_LEN, 0, s32DataBlockLen);
				memcpy(pSendBuf+FRAME_INFO_LEN, pBuf+(i*s32DataBlockLen), s32DataLen);
				pthread_mutex_lock(&spiMut);
				s32SendLen = write(fd, pSendBuf, FRAME_LEN);
				if(s32SendLen != FRAME_LEN)
					printf("[%s]%d: Send ctrlCmd Error(%d)\n", __FILE__, __LINE__, s32SendLen);	
				//send_udp_socket_data(&testSockOpt, pSendBuf, FRAME_LEN);//2018.07.24
				pthread_mutex_unlock(&spiMut);
			}
		}
	} /*while(1)*/
}

/*3.Transmit U Load Cmd*/
void *thread_U_load_cmd(void *ptr)
{
	int fd = *(int *)ptr;
	int i, s32Ret;
	int s32RecvLen=0, s32SendLen=0, s32DataLen=0, s32DataType=TYPE_U_LOAD;
	int s32CntFlag=0, s32CntTime=0, s32DataBlockLen=DATA_MAX_LEN;
	unsigned char pBuf[RECV_BUF_SIZE];
	unsigned char pSendBuf[FRAME_LEN]={FRAME_HEAD_FIR, FRAME_HEAD_SEC};

	/* 1.Socket for Recv Control Cmd */
	Socket_Info_S inSocketOpt;
	inSocketOpt.selCastFlag		=	MULTICAST_RECV;
	inSocketOpt.bindLocalFlag	=	BIND_LOCAL_IP;
	inSocketOpt.pLocalAddr		=	LOCAL_ETH0_IP;
	inSocketOpt.localPort			=	CTRL_U_LOAD_PORT;
	inSocketOpt.pMcastAddr		=	CTRL_CMD_IP;
	s32Ret = init_udp_socket(&inSocketOpt);
	if(s32Ret < 0){
		printf("[%s]%d: Init Socket Error\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		s32RecvLen = recv_udp_socket_data(&inSocketOpt, pBuf, RECV_BUF_SIZE);
		if(s32RecvLen<0){
			printf("[%s]%d: Recv Data Error(%d)\n", __FILE__, __LINE__, s32RecvLen);
		}else{
			//printf("Recv_U_load_cmd = %d\n",s32RecvLen);
			s32CntFlag = (s32RecvLen/(s32DataBlockLen))+1;
			if((s32RecvLen%(s32DataBlockLen))==0){
				s32CntFlag--;
			}			
			s32CntTime = s32CntFlag;

			for(i=0; i<s32CntFlag; i++){
				if(s32CntTime!=1){
					s32DataLen = s32DataBlockLen;
					s32CntTime--;
				}else{
					s32DataLen = s32RecvLen-(i*s32DataBlockLen);
				}

				pSendBuf[POS_DATA_TYPE] = s32DataType;
				pSendBuf[POS_DATA_LEN] = s32DataLen;
				memset(pSendBuf+FRAME_INFO_LEN, 0, s32DataBlockLen);
				memcpy(pSendBuf+FRAME_INFO_LEN, pBuf+(i*s32DataBlockLen), s32DataLen);
				pthread_mutex_lock(&spiMut);
				s32SendLen = write(fd, pSendBuf, FRAME_LEN);
				if(s32SendLen != FRAME_LEN)
					printf("[%s]%d: Send ctrlCmd Error(%d)\n", __FILE__, __LINE__, s32SendLen);	
				//send_udp_socket_data(&testSockOpt, pSendBuf, FRAME_LEN);//2018.07.24
				pthread_mutex_unlock(&spiMut);
			}
		}
	} /*while(1)*/
}

/*4.Transmit Holder Cmd*/
void *thread_holder_cmd(void *ptr)
{
	int fd = *(int *)ptr;
	int i, s32Ret;
	int s32RecvLen=0, s32SendLen=0, s32DataLen=0, s32DataType=TYPE_HOLDER;
	int s32CntFlag=0, s32CntTime=0, s32DataBlockLen=DATA_MAX_LEN;
	unsigned char pBuf[RECV_BUF_SIZE];
	unsigned char pSendBuf[FRAME_LEN]={FRAME_HEAD_FIR, FRAME_HEAD_SEC};

	/* 1.Socket for Recv Control Cmd */
	Socket_Info_S inSocketOpt;
	inSocketOpt.selCastFlag		=	MULTICAST_RECV;
	inSocketOpt.bindLocalFlag	=	BIND_LOCAL_IP;
	inSocketOpt.pLocalAddr		=	LOCAL_ETH0_IP;
	inSocketOpt.localPort			=	CTRL_HOLDER_PORT;
	inSocketOpt.pMcastAddr		=	CTRL_CMD_IP;
	s32Ret = init_udp_socket(&inSocketOpt);
	if(s32Ret < 0){
		printf("[%s]%d: Init Socket Error\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		s32RecvLen = recv_udp_socket_data(&inSocketOpt, pBuf, RECV_BUF_SIZE);
		if(s32RecvLen<0){
			printf("[%s]%d: Recv Data Error(%d)\n", __FILE__, __LINE__, s32RecvLen);
		}else{
			//printf("Recv_holder_cmd = %d\n",s32RecvLen);
			s32CntFlag = (s32RecvLen/(s32DataBlockLen))+1;
			if((s32RecvLen%(s32DataBlockLen))==0){
				s32CntFlag--;
			}			
			s32CntTime = s32CntFlag;

			for(i=0; i<s32CntFlag; i++){
				if(s32CntTime!=1){
					s32DataLen = s32DataBlockLen;
					s32CntTime--;
				}else{
					s32DataLen = s32RecvLen-(i*s32DataBlockLen);
				}

				pSendBuf[POS_DATA_TYPE] = s32DataType;
				pSendBuf[POS_DATA_LEN] = s32DataLen;
				memset(pSendBuf+FRAME_INFO_LEN, 0, s32DataBlockLen);
				memcpy(pSendBuf+FRAME_INFO_LEN, pBuf+(i*s32DataBlockLen), s32DataLen);
				pthread_mutex_lock(&spiMut);
				s32SendLen = write(fd, pSendBuf, FRAME_LEN);
				if(s32SendLen != FRAME_LEN)
					printf("[%s]%d: Send ctrlCmd Error(%d)\n", __FILE__, __LINE__, s32SendLen);	
				//send_udp_socket_data(&testSockOpt, pSendBuf, FRAME_LEN);//2018.07.24
				pthread_mutex_unlock(&spiMut);
			}
		}
	} /*while(1)*/
}

/*5.Transmit Link Cmd*/
void *thread_link_cmd(void *ptr)
{
	int fd = *(int *)ptr;
	int i, s32Ret;
	int s32RecvLen=0, s32SendLen=0, s32DataLen=0, s32DataType=TYPE_LINK;
	int s32CntFlag=0, s32CntTime=0, s32DataBlockLen=DATA_MAX_LEN;
	unsigned char pBuf[RECV_BUF_SIZE];
	unsigned char pSendBuf[FRAME_LEN]={FRAME_HEAD_FIR, FRAME_HEAD_SEC};

	/* 1.Socket for Recv Control Cmd */
	Socket_Info_S inSocketOpt;
	inSocketOpt.selCastFlag		=	MULTICAST_RECV;
	inSocketOpt.bindLocalFlag	=	BIND_LOCAL_IP;
	inSocketOpt.pLocalAddr		=	LOCAL_ETH0_IP;
	inSocketOpt.localPort			=	CTRL_LINK_PORT;
	inSocketOpt.pMcastAddr		=	CTRL_CMD_IP;
	s32Ret = init_udp_socket(&inSocketOpt);
	if(s32Ret < 0){
		printf("[%s]%d: Init Socket Error\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		s32RecvLen = recv_udp_socket_data(&inSocketOpt, pBuf, RECV_BUF_SIZE);
		if(s32RecvLen<0){
			printf("[%s]%d: Recv Data Error(%d)\n", __FILE__, __LINE__, s32RecvLen);
		}else{
			//printf("Recv_link_cmd = %d \n",s32RecvLen);
			s32CntFlag = (s32RecvLen/(s32DataBlockLen))+1;
			if((s32RecvLen%(s32DataBlockLen))==0){
				s32CntFlag--;
			}			
			s32CntTime = s32CntFlag;

			for(i=0; i<s32CntFlag; i++){
				if(s32CntTime!=1){
					s32DataLen = s32DataBlockLen;
					s32CntTime--;
				}else{
					s32DataLen = s32RecvLen-(i*s32DataBlockLen);
				}

				pSendBuf[POS_DATA_TYPE] = s32DataType;
				pSendBuf[POS_DATA_LEN] = s32DataLen;
				memset(pSendBuf+FRAME_INFO_LEN, 0, s32DataBlockLen);
				memcpy(pSendBuf+FRAME_INFO_LEN, pBuf+(i*s32DataBlockLen), s32DataLen);
				pthread_mutex_lock(&spiMut);
				s32SendLen = write(fd, pSendBuf, FRAME_LEN);
				if(s32SendLen != FRAME_LEN)
					printf("[%s]%d: Send ctrlCmd Error(%d)\n", __FILE__, __LINE__, s32SendLen);	
				//send_udp_socket_data(&testSockOpt, pSendBuf, FRAME_LEN);//2018.07.24
				pthread_mutex_unlock(&spiMut);
			}
		}
	} /*while(1)*/
}

/*6.Transmit Param Register*/
void *thread_param_reg(void *ptr)
{
	int fd = *(int *)ptr;
	int i, s32Ret;
	int s32RecvLen=0, s32SendLen=0, s32DataLen=0, s32DataType=TYPE_PARAM_REG;
	int s32CntFlag=0, s32CntTime=0, s32DataBlockLen=DATA_MAX_LEN;
	unsigned char pBuf[RECV_BUF_SIZE];
	unsigned char pSendBuf[FRAME_LEN]={FRAME_HEAD_FIR, FRAME_HEAD_SEC};

	/* 1.Socket for Recv Control Cmd */
	Socket_Info_S inSocketOpt;
	inSocketOpt.selCastFlag		=	MULTICAST_RECV;
	inSocketOpt.bindLocalFlag	=	BIND_LOCAL_IP;
	inSocketOpt.pLocalAddr		=	LOCAL_ETH0_IP;
	inSocketOpt.localPort			=	CTRL_PARAM_REG_PORT;
	inSocketOpt.pMcastAddr		=	CTRL_CMD_IP;
	s32Ret = init_udp_socket(&inSocketOpt);
	if(s32Ret < 0){
		printf("[%s]%d: Init Socket Error\n",__FILE__,__LINE__);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		s32RecvLen = recv_udp_socket_data(&inSocketOpt, pBuf, RECV_BUF_SIZE);
		if(s32RecvLen<0){
			printf("[%s]%d: Recv Data Error(%d)\n", __FILE__, __LINE__, s32RecvLen);
		}else{
			//printf("Recv_param_reg = %d \n",s32RecvLen);
			s32CntFlag = (s32RecvLen/(s32DataBlockLen))+1;
			if((s32RecvLen%(s32DataBlockLen))==0){
				s32CntFlag--;
			}			
			s32CntTime = s32CntFlag;

			for(i=0; i<s32CntFlag; i++){
				if(s32CntTime!=1){
					s32DataLen = s32DataBlockLen;
					s32CntTime--;
				}else{
					s32DataLen = s32RecvLen-(i*s32DataBlockLen);
				}

				pSendBuf[POS_DATA_TYPE] = s32DataType;
				pSendBuf[POS_DATA_LEN] = s32DataLen;
				memset(pSendBuf+FRAME_INFO_LEN, 0, s32DataBlockLen);
				memcpy(pSendBuf+FRAME_INFO_LEN, pBuf+(i*s32DataBlockLen), s32DataLen);
				pthread_mutex_lock(&spiMut);
				s32SendLen = write(fd, pSendBuf, FRAME_LEN);
				if(s32SendLen != FRAME_LEN)
					printf("[%s]%d: Send ctrlCmd Error(%d)\n", __FILE__, __LINE__, s32SendLen);	
				//send_udp_socket_data(&testSockOpt, pSendBuf, FRAME_LEN);//2018.07.24
				pthread_mutex_unlock(&spiMut);
			}
		}
	} /*while(1)*/
}

int main(int argc,char *argv[])
{
	int i, s32Ret;
	pthread_attr_t attr;
	struct sched_param param;
	pthread_t pthreadLPlaneCmd, pthreadUPlaneCmd, pthreadLLoadCmd, pthreadULoadCmd, \
		pthreadHolderCmd, pthreadLinkCmd, pthreadParamReg;
	pthread_t arrThread[CTRL_CMD_CNT]={pthreadLPlaneCmd, pthreadUPlaneCmd, pthreadLLoadCmd,\
		pthreadULoadCmd, pthreadHolderCmd, pthreadLinkCmd, pthreadParamReg};

	/*Init Thread Func*/
	void * (*pFunc[])(void *ptr)={thread_L_plane_cmd, thread_U_plane_cmd, thread_L_load_cmd,\
		thread_U_load_cmd, thread_holder_cmd, thread_link_cmd, thread_param_reg};

	/*Init SPI Interface*/
	int spiFd = open(DM368_DEV_SPI , O_RDWR);
	if (spiFd < 0) {	
		printf("[%s]%d: Open %s Error \n",__FILE__, __LINE__, DM368_DEV_SPI);		
		exit(EXIT_FAILURE);
	}

	/*Init SPI Mutex*/
	pthread_mutex_init(&spiMut, NULL);

	/*Temp call(test)*/ //2018.07.24
	//test_socket_init(&testSockOpt);

	/*Create Thread*/
	for(i=0; i<CTRL_CMD_CNT; i++){
		pthread_attr_init(&attr);
		param.sched_priority =PRIO_BASE_VAL - (PRIO_STEP*i);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		pthread_attr_setschedparam(&attr, &param);
		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

		s32Ret=pthread_create(&arrThread[i], &attr, pFunc[i], &spiFd);
		if(s32Ret != 0){
			printf("[%s]%d: Create pthread[%d] Error\n", __FILE__, __LINE__, i);
			exit(EXIT_FAILURE);
		}
	}

	printf("[%s] begin ... \n", __FILE__);
	for(i=0; i<CTRL_CMD_CNT; i++)
		pthread_join(arrThread[i], NULL);
	
	return 0;
}