LOCAL_DIR := $(GET_LOCAL_DIR)

DEFINES +=	\

LOCAL_OBJS =	\
	board.o

SUBDIRS =

OBJS +=	\
	$(addprefix $(LOCAL_DIR)/,$(LOCAL_OBJS))

include $(foreach subdir,$(SUBDIRS),$(LOCAL_DIR)/$(subdir)/rules.mk)
