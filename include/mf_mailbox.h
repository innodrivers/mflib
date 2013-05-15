#ifndef _MF_MAILBOX_H
#define _MF_MAILBOX_H

typedef u32 mbox_msg_t;

typedef void* mf_mbox_t;
#define MF_MBOX_INVALID		(mf_mbox_t)NULL

extern mf_mbox_t mf_mbox_get(const char* name);
extern int mf_mbox_msg_put(mf_mbox_t _mbox, mbox_msg_t msg);
extern mbox_msg_t mf_mbox_msg_get(mf_mbox_t mbox);

#endif	/* _MF_MAILBOX_H */
