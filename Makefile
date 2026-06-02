obj-m += eats.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules \
		KBUILD_MODPOST_WARN=1 \
		CONFIG_DEBUG_INFO_BTF=n

clean:
	make -C $(KDIR) M=$(PWD) clean
