#
# Makefile for ppacmd
#

CFLAGS += -O2 -I$(KERNEL_INCLUDE_PATH)

all: ppacmd

ppacmd.o: ppacmd.c
ppacmd_autotest.o: ppacmd_autotest.c

ppacmd: ppacmd_autotest.o ppacmd.o

clean:
	rm -f *.o ppacmd