obj-m += kfetch_mod_314551123.o
KDIR ?= $(PWD)/linux-v6.8-devkit

all:
	make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean