LOCAL_DIR := $(GET_LOCAL_DIR)

MIFI_APP := nouse

DEFINES +=	\

LOCAL_OBJS =	\
	mailbox.o	\
	nand.o		\
	netif.o

SUBDIRS =

OBJS +=	\
	$(addprefix $(LOCAL_DIR)/,$(LOCAL_OBJS))

include $(foreach subdir,$(SUBDIRS),$(LOCAL_DIR)/$(subdir)/rules.mk)
