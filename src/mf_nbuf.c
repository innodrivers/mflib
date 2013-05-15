/*
 * mf_netbuf.c
 *
 *  Created on: Apr 25, 2013
 *      Author: drivers
 */

#include <stdio.h>
#include <stdlib.h>

#include <mf_comn.h>
#include <mf_memp.h>
#include <mf_nbuf.h>

#include <libmf.h>
#include "mem.h"

#define MF_NETBUF_PAD		32		//CACHE BYTES

/**
 * @brief alloc a net buffer structure
 *
 *
 * @return : if there is no memory, return NULL,
 * 		otherwise return allocated mf_netbuf pointer.
 *
 */
struct mf_nbuf* mf_netbuf_alloc(void)
{
	struct mf_nbuf *nb;
	void *data;

	nb = mf_memp_alloc(MF_MEMP_NETBUF);
	if (nb == NULL)
		return NULL;

	memset((void*)nb, 0, MF_NETBUF_SIZE);

	data = (void*)((u8*)nb + MF_NETBUF_SIZE);

	nb->head = data;
	nb->data = data;
	mf_netbuf_reset_tail_pointer(nb);

	nb->end = nb->tail + MF_PACKETBUF_SIZE;

	/* set reference count */
	nb->ref = 1;

	mf_netbuf_reserve(nb, MF_NETBUF_PAD);

	return nb;
}

/**
 * @brief increment the reference count of mf_netbuf.
 *
 * @param[in] p : mf_netbuf pointer to increase reference counter of
 *
 * @return : void
 */
void mf_netbuf_ref(struct mf_nbuf* p)
{
	DECL_MF_SYS_PROTECT(oldints);

	if (p != NULL) {
		MF_SYS_PROTECT(oldints);
		++(p->ref);
		MF_SYS_UNPROTECT(oldints);
	}
}

/**
 * @brief decrement the mf_netbuf reference count. if it reaches zero, the
 * 		mf_netbuf is deallocated.
 *
 * @param[in] p : mf_netbuf pointer to be dereferenced
 *
 * @return : void
 */
void mf_netbuf_free(struct mf_nbuf *p)
{
	int ref;
	DECL_MF_SYS_PROTECT(oldints);

	if (p != NULL) {
		MF_SYS_PROTECT(oldints);
		ref = --(p->ref);
		MF_SYS_UNPROTECT(oldints);

		/* this netbuf is no loger referenced to */
		if (ref == 0) {
			/* mf_netbuf is going to free */
			if (p->free_cb)
				p->free_cb(p);

			mf_memp_free(MF_MEMP_NETBUF, p);
		}
	}

}

MF_SYSINIT int mf_netbuf_init(mf_gd_t *gd)
{
	return 0;
}
