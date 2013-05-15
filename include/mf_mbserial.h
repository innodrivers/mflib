#ifndef _MF_MBSERIAL_H
#define _MF_MBSERIAL_H

#include <mf_notifier.h>

#define MAX_MBSERIAL_NUM		8

enum mbserial_notify_event {
	MBSERIAL_EVENT_STARTUP = 0,
	MBSERIAL_EVENT_SHUTDOWN,
	MBSERIAL_EVENT_DATAVAIL,
};


typedef void* mbserial_t;
#define MBSERIAL_INVALID	(mbserial_t)0

extern mbserial_t mf_mbserial_get(int line, struct mf_notifier_block *nb);
extern int mf_mbserial_register_notifier(mbserial_t ser, struct mf_notifier_block *nb);
extern int mf_mbserial_unregister_notifier(mbserial_t ser, struct mf_notifier_block *nb);
extern int mf_mbserial_send(mbserial_t ser, void *data, int len);

#endif	/* _MF_MBSERIAL_H */
