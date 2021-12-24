###################################################################
# $@ : 목표 이름
# $* : 목표 이름에서 확장자가 없는 이름
# $< : 조건 파일 중 첫번재 파일
# $^ : 조건에 있는 모든 파일 이름
###################################################################
###################################################################
# C컴파일러 옵션
# -fallow-single-precision  : Single-precishion 연산을 double-precision 연산으로 변환하지 않음 
# -fcond-mismatch  조건문에서   두 번째와 세 번째 인자의 형이 맞지 않는 것을 허용 
# -fno-signed-bitfields   선언되지 않은 비트 필드를 unsigned형으로 간주 
# -fno-unsigned-bitfields   선언되지 않은 비트 필드를 signed형으로 간주 
# -fno-signed-char  Char 형을 signed형으로 간주 
# -fno-unsigned-char  Char 형을 unsigned형으로 간주 
# -fshort-wchar   Wchar_t 형을 강제로 shot unsigned int 형으로 간주 
# -funsigned-char  Char 형을 unsigned형으로 간주 
###################################################################

CC=$(CROSS_COMPILE)gcc

#LDFLAGS = -s -L./lib -liconv -lasound -lortp -lmediastreamer -lrtpcontrol -I./include 
#LIBS = ./lib/libasound.so.2.0.0 ./lib/libiconv.so.2
CFLAGS  = -g -W -Wall -O0 -l. -lrt -Iinclude -fsigned-char 
INC     = -I.

TARGET  = $(DEVICE_NAME)_app

#OBJS    = main.o app.o \

OBJS	= main.o \
		TcpClient.o

#for make dep
SRCS=$(OBJS:.o=.c)

$(TARGET): $(OBJS)
	$(CC) -lrt -o $@ $^ $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) $(DFLAGS) -c $(CFLAGS) $<

clean:
	rm -rf $(TARGET)* *.o

distclean: clean
	rm -rf .depend

#gccmakedep 대신 -M 옵션으로 처리
dep:
	$(CC) -M $(SRCS) > .depend


#make 시 .depend파일 참조
ifeq (.depend,$(wildcard .depend))
	include .depend
endif

