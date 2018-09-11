/******************************************************************************
 	Program         :	Common Demo of Transmit Interface
 	Author           :	ZhangJ
	Modification	:	2017-9 Created
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/tcp.h>
#include "transmit_interface.h"

/*1.UDP Setup---------------------------*/
/* Init UDP Socket  */
int init_udp_socket(Socket_Info_S *pSockOpt)
{
	int ret = 0;
	int reuse=1;

	// Create UDP Socket Fd
	pSockOpt->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(pSockOpt->sockfd <= 0){
		printf("[%s]%d: Socket Error: %s \n",__func__,__LINE__,strerror(errno));
		return -1;
	}
	ret = setsockopt(pSockOpt->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(ret < 0){
		printf("[%s]%d: Setsockopt Error: %s \n",__func__,__LINE__,strerror(errno));
		close(pSockOpt->sockfd);
		return -1;
	}

	// Set Local Socket Info
	if(pSockOpt->bindLocalFlag == BIND_LOCAL_IP){	
		memset(&pSockOpt->localAddr, 0, sizeof(struct sockaddr_in));
		pSockOpt->localAddr.sin_family = AF_INET;
		if(pSockOpt->pLocalAddr!=NULL){
			pSockOpt->localAddr.sin_addr.s_addr = inet_addr(pSockOpt->pLocalAddr);
		}else{
			pSockOpt->localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		}
		pSockOpt->localAddr.sin_port = htons(pSockOpt->localPort);
		pSockOpt->localLen = sizeof(struct sockaddr_in);
		if(pSockOpt->selCastFlag == MULTICAST_RECV){
			inet_pton(AF_INET, pSockOpt->pMcastAddr, &pSockOpt->localAddr.sin_addr);  //2017.09.13_add
		}

		ret = bind(pSockOpt->sockfd, (struct sockaddr*)&pSockOpt->localAddr, pSockOpt->localLen);
		if(0 != ret){
			printf("[%s]%d: Bind Error: %s \n",__func__,__LINE__,strerror(errno));
			close(pSockOpt->sockfd);
			return -1;
		}
	}

	// Set Remote Socket Info
	switch(pSockOpt->selCastFlag){
	case UNICAST_SEND:
		memset(&pSockOpt->remoteAddr, 0, sizeof(struct sockaddr_in));
		pSockOpt->remoteAddr.sin_family = AF_INET;
		if(pSockOpt->pRemoteAddr!=NULL){
			pSockOpt->remoteAddr.sin_addr.s_addr = inet_addr(pSockOpt->pRemoteAddr); 
		}else{
			pSockOpt->remoteAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
		}
		pSockOpt->remoteAddr.sin_port = htons(pSockOpt->remotePort);
		pSockOpt->remoteLen = sizeof(struct sockaddr_in);
		break;

	case MULTICAST_SEND:
		memset(&pSockOpt->mcastAddr, 0, sizeof(struct sockaddr_in));
		pSockOpt->mcastAddr.sin_family = AF_INET;
		pSockOpt->mcastAddr.sin_addr.s_addr = inet_addr(pSockOpt->pMcastAddr); 
		pSockOpt->mcastAddr.sin_port = htons(pSockOpt->mcastPort);
		pSockOpt->mcastLen = sizeof(struct sockaddr_in);
		break;

	case MULTICAST_RECV:
		memset(&pSockOpt->mcastReq, 0, sizeof(struct ip_mreq));
		pSockOpt->mcastReq.imr_multiaddr.s_addr = inet_addr(pSockOpt->pMcastAddr);
		if(pSockOpt->pLocalAddr!=NULL){
			pSockOpt->mcastReq.imr_interface.s_addr = inet_addr(pSockOpt->pLocalAddr);	
		}else{
			pSockOpt->mcastReq.imr_interface.s_addr = htonl(INADDR_ANY); 
		}
		ret = setsockopt(pSockOpt->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, \
			&pSockOpt->mcastReq,sizeof (struct ip_mreq)); 
		if(ret < 0){
			printf("[%s]%d: Setsockopt Error: %s \n",__func__,__LINE__,strerror(errno));
			close(pSockOpt->sockfd);
			return -1;
		}		
		break;
	default:
		break;
	}

	return 0;
}

/* Send Data (UDP) */
int send_udp_socket_data(Socket_Info_S *pSockOpt, unsigned char * data, unsigned int size)
{
	int realSize;
	if(pSockOpt->selCastFlag == UNICAST_SEND){
		realSize = sendto(pSockOpt->sockfd, data, size, 0, (struct sockaddr*)&pSockOpt->remoteAddr, pSockOpt->remoteLen);
		if(realSize < 0){
			printf("[%s]%d: Unicast Send Error: %s \n",__func__,__LINE__,strerror(errno));
			return -1;
		}
	}else if(pSockOpt->selCastFlag == MULTICAST_SEND){
		realSize = sendto(pSockOpt->sockfd, data, size, 0, (struct sockaddr*)&pSockOpt->mcastAddr, pSockOpt->mcastLen);
		if(realSize < 0){
			printf("[%s]%d: Multicast Send Error: %s \n",__func__,__LINE__,strerror(errno));
			return -1;
		}
	}else {
		printf("[%s]%d: Caller Error \n",__func__,__LINE__);
		return -1;
	}
	return realSize;
}
 
/* Recv Data (UDP) */
int recv_udp_socket_data(Socket_Info_S *pSockOpt, unsigned char * data, unsigned int size)
{
	int realSize;
	if(pSockOpt->selCastFlag == UNICAST_RECV){
		realSize = recvfrom(pSockOpt->sockfd, data, size, 0, (struct sockaddr*)&pSockOpt->remoteAddr, &pSockOpt->remoteLen);
		if(realSize < 0){
			printf("[%s]%d: Unicast Recv Error: %s \n",__func__,__LINE__,strerror(errno));
			return -1;
		}	
	}else if(pSockOpt->selCastFlag == MULTICAST_RECV){
		realSize = recvfrom(pSockOpt->sockfd, data, size, 0, (struct sockaddr*)&pSockOpt->localAddr, &pSockOpt->localLen);
		if(realSize < 0){
			printf("[%s]%d: Multicast Recv Error: %s \n",__func__,__LINE__,strerror(errno));
			return -1;
		}
	}else {
		printf("[%s]%d: Caller Error \n",__func__,__LINE__);
		return -1;
	}

	return realSize;
}

/*2.Tcp Setup---------------------------*/
/* Init TCP Server Socket  */
int init_tcp_server_socket(Socket_Info_S *pSockOpt)
{
	int ret = 0;
	int reuse=1;

	// Create TCP Server Listen Socket Fd
	pSockOpt->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(pSockOpt->sockfd <= 0){
		printf("[%s]%d: Socket Error: %s \n",__func__,__LINE__,strerror(errno));
		return -1;
	}

	ret = setsockopt(pSockOpt->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(ret < 0){
		printf("[%s]%d: Setsockopt Error: %s \n",__func__,__LINE__,strerror(errno));
		close(pSockOpt->sockfd);
		return -1;
	}

	// Set Local Socket Info
	memset(&pSockOpt->localAddr, 0, sizeof(struct sockaddr_in));
	pSockOpt->localAddr.sin_family = AF_INET;
	pSockOpt->localAddr.sin_port = htons(pSockOpt->localPort);
	if(pSockOpt->pLocalAddr!=NULL){
		pSockOpt->localAddr.sin_addr.s_addr = inet_addr(pSockOpt->pLocalAddr);
	}else{
		pSockOpt->localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	pSockOpt->localLen = sizeof(struct sockaddr_in);

	ret = bind(pSockOpt->sockfd, (struct sockaddr*)&pSockOpt->localAddr, pSockOpt->localLen);
	if(0 != ret){
		printf("[%s]%d: Bind Error: %s \n",__func__,__LINE__,strerror(errno));
		close(pSockOpt->sockfd);
		return -1;
	}

	// Listen Client Connect
	ret = listen(pSockOpt->sockfd, 10);
	if(0 != ret){
		printf("[%s]%d: Listen Error: %s \n",__func__,__LINE__,strerror(errno));
		close(pSockOpt->sockfd);
		return -1;
	}

	return 0;
}

/* Wait TCP Client  Connect */
int wait_tcp_client_connect(Socket_Info_S *pSockOpt)
{
	int ret;
	pSockOpt->remoteLen = sizeof(struct sockaddr_in);
	pSockOpt->connfd = accept(pSockOpt->sockfd, (struct sockaddr*)&pSockOpt->remoteAddr, &pSockOpt->remoteLen);

	/* keep alive */
	int keepIdle=1;			// begin heartbeat (5s)
	int keepInterval=1;		// again heartbeat (5s)
	int keepCount=1;		// count=3
	int alive=1;
	ret = setsockopt(pSockOpt->connfd, SOL_SOCKET, SO_KEEPALIVE, &alive, sizeof(alive));
	if(ret<0){
		printf("[%s]%d: Setsockopt Error (keepAlive) \n", __func__,__LINE__);
	}	
	ret = setsockopt(pSockOpt->connfd, SOL_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(keepIdle));
	if(ret<0){
		printf("[%s]%d: Setsockopt Error (keepIdle) \n", __func__,__LINE__);
	}
	ret = setsockopt(pSockOpt->connfd, SOL_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(keepInterval));
	if(ret<0){
		printf("[%s]%d: Setsockopt Error (keepInterval) \n", __func__,__LINE__);
	}
	ret = setsockopt(pSockOpt->connfd, SOL_TCP, TCP_KEEPCNT, &keepCount, sizeof(keepCount));
	if(ret<0){
		printf("[%s]%d: Setsockopt Error (keepCount) \n", __func__,__LINE__);
	}

	return pSockOpt->connfd;
}

/* Send Data (TCP) */ 
int send_tcp_socket_data(int connfd, unsigned char * data, unsigned int size)
{
	int realSize;
	realSize = send(connfd, data, size, MSG_NOSIGNAL);
	if(realSize <= 0){
		//printf("[%s] TCP Send Error \n", __func__);
		close(connfd);
	}
	return realSize;
}

/* Recv Data (TCP) */ 
int recv_tcp_socket_data(int connfd, unsigned char * data, unsigned int size)
{
	int realSize;
	//realSize = recv(connfd, data, size, MSG_NOSIGNAL);
	realSize = recv(connfd, data, size, 0);
	if(realSize <= 0){
		printf("[%s] TCP Recv Error \n", __func__);
		close(connfd);
	}
	return realSize;
}

/* Init TCP Client Socket  */
int init_tcp_client_socket(Socket_Info_S *pSockOpt)
{
	int ret = 0;
	int reuse=1;

	// Create TCP Client Socket
	pSockOpt->sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(pSockOpt->sockfd <= 0){
		printf("[%s]%d: Socket Error: %s \n",__func__,__LINE__,strerror(errno));
		return -1;
	}

	ret = setsockopt(pSockOpt->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
	if(ret < 0){
		printf("[%s]%d: Setsockopt Error: %s \n",__func__,__LINE__,strerror(errno));
		close(pSockOpt->sockfd);
		return -1;
	}

	// Set Server Socket Info
	memset(&pSockOpt->remoteAddr, 0, sizeof(struct sockaddr_in));
	pSockOpt->remoteAddr.sin_family = AF_INET;
	pSockOpt->remoteAddr.sin_port = htons(pSockOpt->remotePort);
	if(pSockOpt->pRemoteAddr != NULL){
		pSockOpt->remoteAddr.sin_addr.s_addr =  inet_addr(pSockOpt->pRemoteAddr);
	}else{
		pSockOpt->remoteAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	pSockOpt->remoteLen = sizeof(struct sockaddr_in);

	return 0;
}

/* TCP Client Connect TCP Server  */
int init_tcp_client_connect(Socket_Info_S *pSockOpt)
{
	int ret = 0;

	//Client Launch Connect
	ret = connect(pSockOpt->sockfd , (struct sockaddr*)&pSockOpt->remoteAddr, pSockOpt->remoteLen); 
	if(ret < 0){
		printf("[%s]%d: Connect Error: %s \n",__func__,__LINE__,strerror(errno));
		close(pSockOpt->sockfd);
		return -1;
	}
	return 0;
}

/* Release  Socket*/
int release_socket(int connfd)
{
	int ret;
	ret = close(connfd);
	if(ret !=0){
		printf("[%s] Close Socket (%d) Error \n", __func__,connfd);
	}
	return ret;
}

/*3.File Setup---------------------------*/
/* Init File  */
int init_file(FILE_OPT_S *pFileOpt)
{
	if((pFileOpt->optMode&O_CREAT) != 0){
		pFileOpt->fd = open(pFileOpt->pFileName, pFileOpt->optMode, 0777);
	}else{
		pFileOpt->fd = open(pFileOpt->pFileName, pFileOpt->optMode);
	}
	if(pFileOpt->fd<0){
		printf("[%s] Open %s Error: %s \n", __func__, pFileOpt->pFileName, strerror(errno));
		return -1;
	}
	return 0;
}

/* Release File  */
void release_file(FILE_OPT_S *pFileOpt)
{
	int s32Ret;
	s32Ret = close(pFileOpt->fd);
	if(s32Ret<0){
		printf("[%s] Close %s Error: %s \n", __func__, pFileOpt->pFileName, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/* Write File  */ 
int write_file(FILE_OPT_S *pFileOpt, unsigned char * data, unsigned int size)
{
	unsigned int realSize = 0;
	realSize=write(pFileOpt->fd, data, size);
	if(realSize < 0){
		printf("[%s] Write %s Error: %s \n",__func__,pFileOpt->pFileName,strerror(errno));
		return -1;
	} 
	return realSize;
}

/* Read File  */  
int read_file(FILE_OPT_S *pFileOpt, unsigned char * data, unsigned int size)
{
	unsigned int realSize = 0;
	realSize=read(pFileOpt->fd, data, size);
	if(realSize<0){ 
		printf("[%s] Read %s Error: %s \n",__func__,pFileOpt->pFileName,strerror(errno));
		return -1;
	} 
	return realSize;
}
