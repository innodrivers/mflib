/*
 * mf_notifier.c
 *
 *  Created on: Apr 27, 2013
 *      Author: drivers
 */
#include <stdio.h>
#include <stdlib.h>

#include <mf_comn.h>
#include <mf_notifier.h>

static int notifier_chain_register(struct mf_notifier_block **nl,
							struct mf_notifier_block *n)
{
	while ((*nl) != NULL) {
		if (n->priority > (*nl)->priority)
			break;
		nl = &((*nl)->next);
	}

	n->next = *nl;
	*nl = n;

	return 0;
}

static int notifier_chain_unregister(struct mf_notifier_block **nl,
							struct mf_notifier_block *n)
{
	while ((*nl) != NULL) {
		if ((*nl) == n) {
			*nl = n->next;
			return 0;
		}
		nl = &((*nl)->next);
	}

	return -1;
}

static int notifier_call_chain(struct mf_notifier_block **nl,
                    unsigned long action, void *data, int len)
{
	int ret = 0;
    struct mf_notifier_block *nb, *next_nb;

    nb = *nl;

    while (nb) {
        next_nb = nb->next;

        ret = nb->notifier_call(nb, action, data, len);

        nb = next_nb;
    }

    return ret;
}


int mf_notifier_chain_register(struct mf_notifier_head *nh, struct mf_notifier_block *n)
{
	int ret;
	DECL_MF_SYS_PROTECT(oldints);

	MF_SYS_PROTECT(oldints);

	ret = notifier_chain_register(&nh->head, n);

	MF_SYS_UNPROTECT(oldints);

	return ret;
}

int mf_notifier_chain_unregister(struct mf_notifier_head *nh, struct mf_notifier_block *n)
{
	int ret;
	DECL_MF_SYS_PROTECT(oldints);

	MF_SYS_PROTECT(oldints);

	ret = notifier_chain_unregister(&nh->head, n);

	MF_SYS_UNPROTECT(oldints);

	return ret;
}

int mf_notifier_call_chain(struct mf_notifier_head *nh, unsigned long action, void *data, int len)
{
	int ret;

	ret = notifier_call_chain(&nh->head, action, data, len);

	return ret;
}
