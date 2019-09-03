MAKEFLAGS += --no-print-directory

KDIR            ?= /lib/modules/$(shell uname -r)/build

subdir-ccflags-y        := -I$(src)/include
obj-m += pcie/

PHONY   := all
all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
