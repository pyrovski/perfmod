obj-m += perfmod.o

ifneq ($(dbg),)
DBG=-D_DEBUG=$(dbg) -g -pg
MYCFLAGS += -O0
else
MYCFLAGS += -O3
endif


all: module test

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

test: test.c
	mpicxx -o test test.c $(MYCFLAGS)

clean:
	rm -f ./test ./*.order ./*.cmd ./*.o ./*.symvers ./*~ ./*.markers
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
