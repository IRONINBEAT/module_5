obj-m += chardev_module.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Некоторые дистрибутивы (RHEL/AlmaLinux) бэкпортируют однопараметрическую
# сигнатуру class_create(const char *) в свои патчи ещё до апстрим-версии
# 6.4, при этом номер ядра остаётся старым (например 5.14) — поэтому
# сигнатуру определяем по реальному заголовку, а не по версии ядра.
CLASS_H := $(KDIR)/include/linux/device/class.h
ccflags-y += $(shell grep -q 'class_create.const char' $(CLASS_H) 2>/dev/null && echo -DCLASS_CREATE_SINGLE_ARG)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
