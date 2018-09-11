#################################
# Program: GDT_COMM_DM368 (for DM368) 
# Author: ZhangJ
# Date: 2018.05.22
#################################
CC = arm-none-linux-gnueabi-gcc
STRIP = arm-none-linux-gnueabi-strip
CFLAGS = -g
COMM_SRC := system_config.c transmit_interface.c transmit_opt_video.c

DEC_SRC 	:= transmit_opt_dec_frame.c
DEC_OBJ 	:= $(DEC_SRC:%.c=%.o)
DEC_TARGET	:= traDecFrame

CTRL_SRC 	:= transmit_opt_remote_ctrl.c
CTRL_OBJ 	:= $(CTRL_SRC:%.c=%.o)
CTRL_TARGET	:= traRemCtrl

MON_SRC 	:= monitor_online.c
MON_OBJ 	:= $(MON_SRC:%.c=%.o)
MON_TARGET	:= monitor_online

all:
	$(CC) -lpthread -o $(DEC_TARGET) $(DEC_SRC) $(COMM_SRC) $(CFLAGS)
	$(STRIP) -s $(DEC_TARGET)
	$(CC) -lpthread -o $(CTRL_TARGET) $(CTRL_SRC) $(COMM_SRC) $(CFLAGS)
	$(STRIP) -s $(CTRL_TARGET)
	$(CC) -lpthread -o $(MON_TARGET) $(MON_SRC) $(COMM_SRC) $(CFLAGS)
	$(STRIP) -s $(MON_TARGET)	

clean:
	@rm -f $(DEC_TARGET)
	@rm -f $(CTRL_TARGET)
	@rm -f $(MON_TARGET)	
	
	
	
	
	