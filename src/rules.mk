LOCAL_DIR := $(GET_LOCAL_DIR)

LOCAL_OBJS =	\
	iomux.o		\
	mf_core.o	\
	mf_debug.o	\
	mf_thread.o	\
	mf_mbnetif.o	\
	mf_mbnand.o	\
	mf_mbserial.o	\
	micproto.o	\
	mreqb.o	\
	mf_bootcpu2.o	\
	mbser_log.o		\
	mbser_at.o		\
	cpu2_boot.o		\
	mf_memp.o		\
	mf_nbuf.o		\
	mreqb_reservep.o	\
	mreqb_fifo.o	\
	nbuf_queue.o	\
	mf_notifier.o

OBJS +=	\
	$(addprefix $(LOCAL_DIR)/,$(LOCAL_OBJS))

DEFINES +=		\
	CONFIG_P4A_CPU1=1	\
