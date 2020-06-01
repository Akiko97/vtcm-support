# Comment/uncomment the following line to enable/disable debugging
#DEBUG = y


ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DSCULLP_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif


main-OBJECTS = main.o 
  

#CFLAGS += $(DEBFLAGS) -I$(LDDINC) -I"../misc/"
VPATH = .:../misc

total-OBJECTS = $(main-OBJECTS)  

EXTRA_CFLAGS += -I$(INC) -I$(SELF_PATH) 

TARGET = LSMHookM

ifneq ($(KERNELRELEASE),)

LSMHookM-objs := $(total-OBJECTS)
obj-m	:= LSMHookM.o
else

PRIVATE_INC = $(shell pwd)/../include 
__SELF = $(shell pwd)/
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) INC=$(PRIVATE_INC) SELF_PATH=$(__SELF) modules
endif


install:
	install -d $(INSTALLDIR)
	install -c $(TARGET).o $(INSTALLDIR)

clean:
	rm -rf *.o $(total-OBJECTS) *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions modules.order  Module.symvers


depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
