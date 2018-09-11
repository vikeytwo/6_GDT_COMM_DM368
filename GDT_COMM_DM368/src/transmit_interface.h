/******************************************************************************
 	Program         :	Common Demo of Transmit Interface
 	Author           :	ZhangJ
	Modification	:	2018-1 Created
******************************************************************************/
#ifndef  __TRANSMIT_INTERFACE_H__
#define __TRANSMIT_INTERFACE_H__
#include <netdb.h>
#include <fcntl.h>

typedef enum{
	UNBIND_LOCAL_IP = 0,
	BIND_LOCAL_IP
}BIND_LOCAL_FLAG_E;

typedef enum{
	UNICAST_RECV = 0,
	UNICAST_SEND,
	MULTICAST_RECV,
	MULTICAST_SEND
}SEL_CAST_FLAG_E;

typedef struct Socket_Information{
	SEL_CAST_FLAG_E		selCastFlag;	
	BIND_LOCAL_FLAG_E	bindLocalFlag; /* if selCastFlag=xxx_RECV, bindLocalSockFlag = 1*/

	int								sockfd;
	int								connfd;
	const char					*pLocalAddr;
	unsigned int				localPort;
	const char					*pRemoteAddr;
	unsigned int				remotePort;
	const char					*pMcastAddr;
	unsigned int				mcastPort;

	struct sockaddr_in		localAddr;
	socklen_t					localLen;
	struct sockaddr_in		remoteAddr;
	socklen_t					remoteLen;
	struct sockaddr_in		mcastAddr;
	socklen_t					mcastLen;
	struct ip_mreq				mcastReq;
}Socket_Info_S; 

int init_udp_socket(Socket_Info_S *pSockOpt);
int send_udp_socket_data(Socket_Info_S *pSockOpt, unsigned char * data, unsigned int size);
int recv_udp_socket_data(Socket_Info_S *pSockOpt, unsigned char * data, unsigned int size);

int init_tcp_server_socket(Socket_Info_S *pSockOpt);
int wait_tcp_client_connect(Socket_Info_S *pSockOpt);
int init_tcp_client_socket(Socket_Info_S *pSockOpt);
int init_tcp_client_connect(Socket_Info_S *pSockOpt);
int send_tcp_socket_data(int connfd, unsigned char * data, unsigned int size);
int recv_tcp_socket_data(int connfd, unsigned char * data, unsigned int size);

// File 
typedef struct File_Opt{
	int			fd;
	int			optMode;
	char *	pFileName;
}FILE_OPT_S;

int init_file(FILE_OPT_S *pFileOpt);
void release_file(FILE_OPT_S *pFileOpt);
int write_file(FILE_OPT_S *pFileOpt, unsigned char * data, unsigned int size);
int read_file(FILE_OPT_S *pFileOpt, unsigned char * data, unsigned int size);

#endif /* End of #ifndef __TRANSMIT_INTERFACE_H__ */