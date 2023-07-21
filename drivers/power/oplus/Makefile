ifeq ($(CONFIG_OPLUS_CHARGER_MTK),y)
subdir-ccflags-y += -Werror
subdir-ccflags-y += -Wno-unused-variable
ifeq ($(CONFIG_OPLUS_CHARGER_MTK6779Q),y)
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include
endif
endif

obj-y      += charger_ic/
obj-y      += gauge_ic/
obj-y      += vooc_ic/
obj-y      += adapter_ic/

ifeq ($(CONFIG_OPLUS_CHARGER_MTK6769),y)
obj-y	+= oplus_bq_charger.o
else
obj-y	+= oplus_charger.o
endif
obj-y	+= oplus_gauge.o
obj-y	+= oplus_vooc.o
obj-y	+= oplus_adapter.o
obj-y	+= oplus_short.o
obj-y   += oplus_debug_info.o
ifeq ($(CONFIG_OPLUS_CHARGER_MTK6785),y)
obj-y	+= oplus_usb_ntc1.o
obj-y	+= oplus_usb_ntc2.o
obj-y	+= oplus_flashlight_ntc.o
obj-y	+= oplus_charger_ntc.o
endif
