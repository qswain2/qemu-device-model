obj-m := linux/hello.o

KDIR?=/lib/modules/`uname -r`/build

all: modules


modules:
	$(MAKE) -C $(KDIR) M=$(PWD)

modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
