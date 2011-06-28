obj-m += perfmod.o
all: module test

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

test: test.c

clean:
	rm -f ./test ./*.order ./*.cmd ./*.o ./*.symvers ./*~ ./*.markers
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
