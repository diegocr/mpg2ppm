# Makefile for mpg2ppm

PACKAGE	= mpg2ppm

WARNINGS= #-Wall
VERIFY	= #-DVERIFY

# GNU gcc
CC	= gcc
CFLAGS	= $(VERIFY) $(WARNINGS) $(CPU) -O2 $(OPT)
LIBS	= -lm

OBJS	= \
mpg2ppm$(SYS).o getpic$(SYS).o motion$(SYS).o getvlc$(SYS).o gethdr$(SYS).o \
getblk$(SYS).o getbits$(SYS).o store$(SYS).o recon$(SYS).o spatscal$(SYS).o  \
idct$(SYS).o idctref$(SYS).o systems$(SYS).o subspic$(SYS).o verify$(SYS).o

%$(SYS).o: config.h global.h mpg2ppm.h %.c
	$(CC) $(CFLAGS) -c -o $@ $(@:$(SYS).o=.c)

all:	$(OBJS)
	$(CC) $(CFLAGS) -o $(PACKAGE)$(SYS) $(OBJS) $(LIBS)



# To compile for AmigaOS (m68k)
amiga:
	$(MAKE) SYS='_020' CPU='-m68020-60' OPT='-noixemul -msmall-code' --no-print-directory
	$(MAKE) SYS='_040' CPU='-m68040 -m68881' OPT='-noixemul -msmall-code' --no-print-directory
	$(MAKE) SYS='_060' CPU='-m68060' OPT='-noixemul -msmall-code' --no-print-directory


# To compile for AmigaOS (ppc)
wos:
	$(MAKE) CC='ppc-amigaos-gcc -warpup' SYS='_Wos' CPU='-mmultiple -mcpu=603e' --no-print-directory


# To compile for MorphOS
mos:
	$(MAKE) CC='ppc-morphos-gcc' SYS='_MOS' CPU='-mmultiple -mcpu=603e' OPT='-noixemul' --no-print-directory


clean:
	rm -f *.o
