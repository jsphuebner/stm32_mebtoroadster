##
## Makefile for stm32_mebtoroadster host-side unit tests
##

CC       = gcc
CPP      = g++
LD       = g++
CFLAGS   = -std=c99  -ggdb -Wall -fpermissive -DSTM32F1 -DMAX_ITEMS=154 -DMAX_MESSAGES=24 \
           -Itest-include -I../include -I../libopeninv/include -I../libopencm3/include
CPPFLAGS = -std=c++14 -ggdb -Wall -fpermissive -DSTM32F1 -DMAX_ITEMS=154 -DMAX_MESSAGES=24 \
           -Itest-include -I../include -I../libopeninv/include -I../libopencm3/include \
           -I.
LDFLAGS  = -g
BINARY   = test_roadsterbmb

# Object files: project sources + libopeninv dependencies + test files
OBJS = test_main.o \
       test_roadsterbmb.o \
       roadsterbmb.o \
       mebbms.o \
       params.o \
       canmap.o \
       canhardware.o \
       errormessage.o \
       printf.o \
       picontroller.o \
       my_fp.o \
       my_string.o \
       stub_libopencm3.o

VPATH = ../src:../libopeninv/src

all: $(BINARY)

$(BINARY): $(OBJS)
	$(LD) $(LDFLAGS) -o $(BINARY) $(OBJS) -lm

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(OBJS) $(BINARY)

.PHONY: all clean
