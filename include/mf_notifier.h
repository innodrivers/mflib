/*
 * mf_notifier.h
 *
 *  Created on: Apr 27, 2013
 *      Author: drivers
 */

#ifndef MF_NOTIFIER_H_
#define MF_NOTIFIER_H_


struct mf_notifier_block {
	int (*notifier_call)(struct mf_notifier_block *nb, unsigned long action, void *data, int len);
	struct mf_notifier_block *next;
	int priority;
};

struct mf_notifier_head {
	struct mf_notifier_block *head;
};

#define INIT_MF_NOTIFIER_HEAD(name)	do {	\
		(name)->head = NULL;				\
	}while (0)

#define MF_NOTIFIER_HEAD(name)			\
	struct mf_notifier_block name = 	\
		INIT_MF_NOTIFIER_HEAD(name)

extern int mf_notifier_chain_register(struct mf_notifier_head *nh, struct mf_notifier_block *n);
extern int mf_notifier_chain_unregister(struct mf_notifier_head *nh, struct mf_notifier_block *n);
extern int mf_notifier_call_chain(struct mf_notifier_head *nh, unsigned long action, void *data, int len);

#endif /* MF_NOTIFIER_H_ */
