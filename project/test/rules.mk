LOCAL_DIR := $(GET_LOCAL_DIR)

MIFI_LIB := libmf
MIFI_APP := mfstub

DEFINES +=	\

LOCAL_OBJS =	\
	main.o		\
	nand_stub.o	\
	p4a_mbox.o	\
	usbwan.o	\
	usbnet.o


SUBDIRS =

OBJS +=	\
	$(addprefix $(LOCAL_DIR)/,$(LOCAL_OBJS))

include $(foreach subdir,$(SUBDIRS),$(LOCAL_DIR)/$(subdir)/rules.mk)
