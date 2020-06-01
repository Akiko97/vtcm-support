BUILD ?= /lib/modules/`uname -r`/build

FtraceHookM-objs := fhm.o
obj-m := FtraceHookM.o

all:
	$(MAKE) -C $(BUILD) M=$(PWD) modules;

.PHONY: clean
clean:
	$(MAKE) -C $(BUILD) M=$(PWD) clean;
	rm -f *.ko
