/******************************************************************************
 	Program         :	HA of Monitor Online
 	Author           :	ZhangJ
	Modification	:	2018-04 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/stat.h>

#include "system_config.h"

#define SUB_PROCESS_SIZE			5
#define FAIL_LIMIT_VALUE			10
#define RTSP_SERVER					"./testH265VideoStreamer"
#define RTSP_FRAME_RATE			"60"

int pidArray[SUB_PROCESS_SIZE];

char *taskVideoArgFir[]	=	{RTSP_SERVER, V0_FIFO_NAME, V0_STREAM_NAME, V0_PORT, RTSP_FRAME_RATE, NULL};
char *taskVideoArgSec[]	=	{RTSP_SERVER, V1_FIFO_NAME, V1_STREAM_NAME, V1_PORT, RTSP_FRAME_RATE, NULL};
char *taskVideoArgThi[]	=	{RTSP_SERVER, V2_FIFO_NAME, V2_STREAM_NAME, V2_PORT, RTSP_FRAME_RATE, NULL};

/* Launch Sub Process */
void init_sub_process(int flag)
{
	switch(flag){
	case 0:
		execv(RTSP_SERVER,taskVideoArgFir);
		break;
	case 1:
		execv(RTSP_SERVER, taskVideoArgSec);
		break;
	case 2:
		execv(RTSP_SERVER, taskVideoArgThi);
		break;
	case 3:
		execv("./traDecFrame", NULL);
		break;
	case 4:
		execv("./traRemCtrl", NULL);
		break;
	default:
		printf("[%s]%d: Never Happen Flag=%d (%s)\n", __FILE__,__LINE__,flag,__func__);
		exit(flag);
	}
	exit(flag);
}

/* Handle Quit Signal */
void signal_handle_fun(int sig)
{
	int i, ret, status;
	static int sumQuitCnt=0;
	static int lastQuitFlag=0;
	pid_t pid, quitPid;
	quitPid = wait(&status);
	for(i=0; i<SUB_PROCESS_SIZE; i++){
		if(quitPid == pidArray[i]){
			break;			
		}
	}
	printf("[%s] Quit Pid=%d, Ret=%d (i=%d)\n", __FILE__,quitPid, WEXITSTATUS(status),i);

	//2018.04.19
	sumQuitCnt++;
	if(sumQuitCnt==FAIL_LIMIT_VALUE){
		exit(-1);
	}
	//if(lastQuitFlag != i){
	//	lastQuitFlag = i;
	//	sumQuitCnt = 1;
	//}else{
	//	sumQuitCnt++;
	//	if(sumQuitCnt==FAIL_LIMIT_VALUE){
	//		exit(-1);
	//	}
	//}
	
	/* Create Sub Process */
	pid=fork();
	if(-1 == pid){							
		printf("[%s]%d: Init Fork Error \n",__FILE__,__LINE__);
		exit(-1);
	}else if(0==pid){
		prctl(PR_SET_PDEATHSIG, SIGHUP);
		init_sub_process(i);
	}else{	
		pidArray[i] = pid;
	}
}

void mkfifo_judge(char *fifoName)
{
	int ret;
	if(access(fifoName, F_OK)<0){
		ret = mkfifo(fifoName, 0777);
		if(ret <0){
			printf("[%s] mkfifo %s error \n", __FILE__, fifoName);
			exit(EXIT_FAILURE);
		}
	}
}

int main(void)
{
	int i,ret;
	pid_t  pid;	

	/*1.Create Fifo*/
	mkfifo_judge(V0_FIFO_NAME);
	mkfifo_judge(V1_FIFO_NAME);
	mkfifo_judge(V2_FIFO_NAME);
	
	/*2.Create Sub Process*/
	for(i=0; i<SUB_PROCESS_SIZE; i++){
		
		pid = fork();	
		if(-1 == pid){							
			printf("[%s]%d: Init Fork Error \n",__FILE__,__LINE__); 
			exit(-1);	
		}else if(0==pid){
			usleep(100*1000);	//2018.04.19
			prctl(PR_SET_PDEATHSIG, SIGHUP);	
			init_sub_process(i);
		}else{
			pidArray[i] = pid;		//2018.04.19
		}
	}

	/*3.Wait Sub Process Quit Signal */
	signal(SIGCHLD, signal_handle_fun);
	while(1)
		pause();

	return 0;
}


















