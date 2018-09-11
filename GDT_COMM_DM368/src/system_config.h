/******************************************************************************
 	Program         :	GDT_COMM_DM368 of System Config 
 	Author           :	ZhangJ
	Modification	:	2018-5 Created
******************************************************************************/
#ifndef  __SYSTEM_CONFIG_H__
#define __SYSTEM_CONFIG_H__

#define malloc_mem(type,n)  (type *)malloc((n)*sizeof(type)) 

/* LOCAL Network Info */		
#define LOCAL_ETH0_IP								NULL  /*def: local-eth*/   /*"192.168.1.43"*/

/* Input Control Command */
#define	CTRL_CMD_IP									"224.0.1.2"
#define	CTRL_CMD_CNT								7

/* Input Control Command Dest Port*/
#define	CTRL_L_PLANE_PORT						10000
#define CTRL_U_PLANE_PORT						10010
#define	CTRL_L_LOAD_PORT						10002
#define CTRL_U_LOAD_PORT						10012
#define CTRL_HOLDER_PORT						10020
#define CTRL_LINK_PORT								10022
#define CTRL_PARAM_REG_PORT					10024

/* Output Parse Data */
#define PARSE_DATA_IP								"226.0.1.2"
#define OUTPUT_DATA_CNT							11

/* Output Parse Data Output and Local Port */
#define NUM_L_PLANE									0
#define OUTPUT_L_PLANE_PORT					9000		/*0. L-·É¿Ø»Ø±¨*/
#define LOCAL_L_PLANE_PORT						20000

#define NUM_L_LOAD									1
#define OUTPUT_L_LOAD_PORT					9002		/*1. L-ÔØºÉ»Ø±¨*/
#define LOCAL_L_LOAD_PORT						20002

#define NUM_LL_LINK_MEAS							2
#define OUTPUT_LL_LINK_MEAS_PORT			9004		/*2. L-LÁ´²â*/
#define LOCAL_LL_LINK_MEAS_PORT				20004

#define NUM_LU_LINK_MEAS						3
#define OUTPUT_LU_LINK_MEAS_PORT			9006		/*3. L-UÁ´²â*/
#define LOCAL_LU_LINK_MEAS_PORT				20006

#define NUM_L_STAT_MEAS							4
#define OUTPUT_L_STAT_MEAS_PORT			9008		/*4. L-Õ¾²â*/
#define LOCAL_L_STAT_MEAS_PORT				20008

#define NUM_U_PLANE									5
#define OUTPUT_U_PLANE_PORT					9010		/*5. U-·É¿Ø»Ø±¨*/
#define LOCAL_U_PLANE_PORT						20010

#define NUM_U_LOAD									6
#define OUTPUT_U_LOAD_PORT					9012		/*6. U-ÔØºÉ»Ø±¨*/
#define LOCAL_U_LOAD_PORT						20012

#define NUM_UU_LINK_MEAS						7
#define OUTPUT_UU_LINK_MEAS_PORT			9014		/*7. U-UÁ´²â*/
#define LOCAL_UU_LINK_MEAS_PORT			20014

#define NUM_U_STAT_MEAS							8
#define OUTPUT_U_STAT_MEAS_PORT			9016		/*8. U-Õ¾²â*/
#define LOCAL_U_STAT_MEAS_PORT				20016

#define NUM_UL_LINK_MEAS						9
#define OUTPUT_UL_LINK_MEAS_PORT			9018		/*9. U-LÁ´²â*/
#define LOCAL_UL_LINK_MEAS_PORT				20018

#define NUM_HOLDER									10
#define OUTPUT_HOLDER_PORT					9020		/*10. ÔÆÌ¨»Ø±¨*/
#define LOCAL_HOLDER_PORT						20020

/* DM368 GPIO Control */
enum {
	DM368_SET_GPIO45,
	DM368_CLEAR_GPIO45,
	DM368_SET_GPIO49,
	DM368_CLEAR_GPIO49,

	DM368_GET_GPIO29,
	DM368_GET_GPIO44,
	DM368_GET_GPIO46,
	DM368_GET_GPIO47,
	DM368_GET_GPIO48,
};
enum davinci_dm365_gpio_to_fpga_index {
	DM365_GPIO49,
	DM365_GPIO48,
	DM365_GPIO47,
	DM365_GPIO46,
	DM365_GPIO45,
	DM365_GPIO44,
	DM365_GPIO29,
};

/*EMIF Sync Flag*/
#define EMIF_READ_FLAG								DM368_GET_GPIO44
#if 0		/*ZSE*/
#define EMIF_READ_SIZE_FIR_FLAG				DM368_GET_GPIO29	
#define EMIF_READ_SIZE_SEC_FLAG				DM368_GET_GPIO48
# else	/*V3.0*/ 
#define EMIF_READ_SIZE_FIR_FLAG				DM368_GET_GPIO46	
#define EMIF_READ_SIZE_SEC_FLAG				DM368_GET_GPIO47
#endif

/* DM368 Comm Interface */
#define DM368_DEV_GPIO							"/dev/gpio_ctrl"
#define DM368_DEV_EMIF							"/dev/emif_edma"
#define DM368_DEV_UART							"/dev/ttyS1"   
#define DM368_DEV_SPI								"/dev/spidev0.0" 

/*Memory for Decode Frame*/
#define CONFIG_RECV_LEN							(64*1024)	//(64<<10)    /*64KB*/
#define CONFIG_RESULT_LEN						(2*1024)	//(2<<10)      /*2KB*/
#define CONFIG_VIDEO_LEN							(256*1024)	//(256<<10)   /*256KB*/
#define SIZE_COMPLEX_A							1167								/*1167B*/
#define SIZE_COMPLEX_B							221								/*221B*/
#define SIZE_CTRL										72									/*72B*/

/* Pick Frame Head Opt */
typedef struct Pick_Frame_Opt{
	unsigned char	*		pBuff;
	int							buffLen;
	unsigned char	*		pFrontMatch;
	int							frontLen;
	unsigned char	*		pRearMatch;
	int							rearLen;
}PICK_FRAME_S;

//typedef enum{
//	NULL_DATA	= 0x00,
//	VIDEO_FIR		= 0x01,
//	VIDEO_SEC		= 0x02,
//	VIDEO_THI		= 0x04,
//	SYNC_DATA	= 0x08
//}LOAD_DATA_FLAG_E;

typedef enum{
	DATA_RATE_ZERO	= 512,
	DATA_RATE_FIR		= (1*1024),
	DATA_RATE_SEC		= (2*1024),
	DATA_RATE_THR		= (4*1024),
}EMIF_READ_SIZE_E;

typedef enum{
	VALID	= 0,
	INVALID	= 1,
}OPT_FLAG_E;

typedef struct Node{
	int data;
	struct Node *next;
}NODE_S;

/*Video Info*/
#define VIDEO_NUM			3

/*Task Video Info*/
#define V0_FIFO_NAME		"task_fifo1"
#define V0_STREAM_NAME	"testStream"
#define V0_PORT				"11001"

#define V1_FIFO_NAME		"task_fifo2"
#define V1_STREAM_NAME	"testStream"
#define V1_PORT				"12001"

#define V2_FIFO_NAME		"task_fifo3"
#define V2_STREAM_NAME	"testStream"
#define V2_PORT				"13001"

/*Video Info (Global Declare)*/
extern unsigned char *pVideoBuf[VIDEO_NUM];
extern int writeFlag[VIDEO_NUM];
extern int s32VideoLen[VIDEO_NUM];
extern pthread_mutex_t videoMut[VIDEO_NUM];
extern pthread_cond_t videoCond[VIDEO_NUM];

/*system_config*/
int pick_frame_head(PICK_FRAME_S *opt, unsigned char *frameHead, int frameHeadLen);
int detect_emif_read_flag(int fd);
int detect_emif_read_size(int fd);

/*transmit_opt_video*/
void *thread_pick_zero_video(void *ptr);
void *thread_pick_first_video(void *ptr);
void *thread_pick_second_video(void *ptr);
#endif /* End of #ifndef __SYSTEM_CONFIG_H__ */
