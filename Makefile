obj-m += perfmod.o

ifneq ($(dbg),)
DBG=-D_DEBUG=$(dbg) -g -pg
MYCFLAGS += -O0
else
MYCFLAGS += -O3
endif


all: module test omp_test

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

test: test.c
	mpicc -o $@ $^ $(MYCFLAGS)

omp_test: omp_test.c
	gcc -o $@ $^ $(MYCFLAGS) -fopenmp

clean:
	rm -f ./test ./*.order ./*.cmd ./*.o ./*.symvers ./*~ ./*.markers
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install: module
	sudo ln -f -s `pwd`/perfmod.ko /lib/modules/`uname -r`/
	sudo depmod -a
