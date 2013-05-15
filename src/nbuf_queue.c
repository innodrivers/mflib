/**
 *
 * mf_nbuf FIFO queue utils
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		mf_nbuf FIFO queue
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *
 * @date:	2013-04-26
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>

#include <mf_comn.h>
#include <mf_memp.h>
#include <mf_nbuf.h>
#include <nbuf_queue.h>


/**
 * @brief create a First-In-First-Out Queue of mf_nbuf
 *
 * @return a pointer of nbuf_queue_t
 */
mf_nbuf_queue_t * mf_nbuf_queue_create(void)
{
	mf_nbuf_queue_t *q;

	q = (mf_nbuf_queue_t *)malloc(sizeof(*q));
	if (q == NULL)
		return NULL;

	memset(q, 0, sizeof(*q));
	q->last = &q->head;
	q->elem_num = 0;
	cyg_flag_init(&q->event);

	return q;
}

/**
 * @brief put a mf_nbuf into the Queue
 *
 * @param[in] q : the mf_nbuf queue
 * @param[in] nbuf : the mf_nbuf pointer
 *
 * @return : void
 */
void mf_nbuf_queue_put(mf_nbuf_queue_t *q, struct mf_nbuf *nbuf)
{
	DECL_MF_SYS_PROTECT(oldints);

	MF_SYS_PROTECT(oldints);

	*(q->last) = nbuf;
	q->last = &(nbuf->next);

	q->elem_num++;

	if (q->elem_num == 1)
		cyg_flag_setbits(&q->event, 0x1);

	MF_SYS_UNPROTECT(oldints);
}

/**
 * @brief get a mf_nbuf from the Queue
 *
 * @param[in] q : the mf_nbuf queue
 *
 * @return : a mf_nbuf pointer, if Queue empty, then NULL return.
 */
struct mf_nbuf * mf_nbuf_queue_get(mf_nbuf_queue_t *q)
{
	DECL_MF_SYS_PROTECT(oldints);
	struct mf_nbuf *nbuf = NULL;

	MF_SYS_PROTECT(oldints);

	nbuf = q->head;
	if (nbuf != NULL) {
		q->head = nbuf->next;
		if (q->head == NULL) {
			q->last = &(q->head);
		}
		nbuf->next = NULL;

		q->elem_num--;
	}

	MF_SYS_UNPROTECT(oldints);

	return nbuf;
}

/**
 * @brief get a mf_nbuf from the Queue, if queue empty, will wait util queue available
 *
 * @param[in] q : the mf_nbuf queue
 *
 * @return : a mf_nbuf pointer
 */
struct mf_nbuf * mf_nbuf_queue_waited_get(mf_nbuf_queue_t *q)
{
	struct mf_nbuf *nbuf;

	do {
		nbuf = mf_nbuf_queue_get(q);
		if (nbuf == NULL) {
			cyg_flag_wait(&q->event, 0x1, CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR);
			continue;
		}

		break;

	} while (1);

	return nbuf;
}
