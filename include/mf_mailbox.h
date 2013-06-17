#ifndef _MF_MAILBOX_H
#define _MF_MAILBOX_H

typedef u32 mbox_msg_t;

typedef void* mf_mbox_t;
#define MF_MBOX_INVALID		(mf_mbox_t)NULL

extern mf_mbox_t mf_mbox_get(const char* name);
extern int mf_mbox_msg_put(mf_mbox_t _mbox, mbox_msg_t msg);
extern mbox_msg_t mf_mbox_msg_get(mf_mbox_t mbox);




#define MESSAGE_TYPE_EVENT  0x0
#define MESSAGE_TYPE_MSG    0x1
#define MESSAGE_TYPE_MSK    (0x1 << 31)

#define CHIP_TYPE_DSP       0x0
#define CHIP_TYPE_ARM_M     0x1
#define CHIP_TYPE_ARM_S     0x2
#define CHIP_TYPE_MSK       (0x3 << 29)

#define MEMERY_TYPE_DDR     0x1
#define MEMERY_TYPE_OTH     0x0
#define MEMERY_TYPE_MSK     (0x1 << 28)

#define MESSAGE_HEAD_MSK    (0xF << 28)

#define MESSAGE_HEAD_ARM_S  ((MESSAGE_TYPE_MSG << 31) | (CHIP_TYPE_ARM_S << 29))

#define MESSAGE_HEAD_ARM_M  ((MESSAGE_TYPE_MSG << 31) | (CHIP_TYPE_ARM_M << 29))

#define MESSAGE_HEAD    ((MESSAGE_TYPE_MSG << 31) | (CHIP_TYPE_ARM_M << 29))

#define COVER_TO_ADDR(msg)  (((msg) & (~MESSAGE_HEAD_MSK)) + 0x40000000)
#define COVER_TO_MSG(addr)  (((addr) & (~MESSAGE_HEAD_MSK)) + MESSAGE_HEAD)

static inline mbox_msg_t out_msg_fixup(mbox_msg_t msg)
{
	return COVER_TO_MSG(msg);
}

static inline mbox_msg_t in_msg_fixup(mbox_msg_t msg)
{
	return COVER_TO_ADDR(msg);
}

#endif	/* _MF_MAILBOX_H */
