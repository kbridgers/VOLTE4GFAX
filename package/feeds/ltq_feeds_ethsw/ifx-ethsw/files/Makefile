SRC	= src
#SRCS    :=$(wildcard $(SRC)/*.c)
#OBJS    :=$(patsubst %.c, %.o,$(SRCS))

SRCS    :=$(wildcard $(SRC)/switch_utility.c)
OBJS    :=$(patsubst switch_utility.c, switch_utility.o,$(SRCS))

SRCS1    :=$(wildcard $(SRC)/switch_utility_int.c)
OBJS1    :=$(patsubst switch_utility_int.c, switch_utility_int.o,$(SRCS1))
CFLAGS = $(IFX_CFLAGS)
CFLAGS += -fPIC -Iinclude -DVERBOSE_PRINT

all: switch_utility switch_utility_int

switch_utility:$(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
switch_utility_int:$(OBJS1)
	$(CC) $(CFLAGS) -o $@ $^
clean:
#	-rm -f *.o $(SRC)/*.o switch_utility
	-rm -f switch_utility.o $(SRC)/switch_utility.o switch_utility
	-rm -f switch_utility_int.o $(SRC)/switch_utility_int.o switch_utility
ifxinstall:
	$(STRIP) switch_utility
	$(STRIP) switch_utility_int

