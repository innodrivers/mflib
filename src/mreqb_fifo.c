/**
 *
 * mreqb FIFO queue management
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		mreqb FIFO queue
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *
 * @date:	2013-04-24
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h> 
#include <cyg/kernel/kapi.h>

#include <micproto.h>
#include <micp_cmd.h>
#include <mf_debug.h>
#include "mreqb_fifo.h"

struct mreqb_fifo {
	struct list_head list;
	cyg_flag_t	wakeup;
	cyg_mutex_t lock;
};

/**
 * @brief create a mreqb FIFO
 *
 * @return a mreqb FIFO instance
 */
mreqb_fifo_t mreqb_fifo_create(void)
{
	struct mreqb_fifo *fifo;

	fifo = (struct mreqb_fifo *)malloc(sizeof(*fifo));
	if (fifo == NULL)
		return (mreqb_fifo_t)NULL;

	INIT_LIST_HEAD(&fifo->list);
	cyg_mutex_init(&fifo->lock);
	cyg_flag_init(&fifo->wakeup);

	return (mreqb_fifo_t)fifo;
}

/**
 * @brief put a mreqb into the FIFO
 *
 * @param[in] _fifo : the mreqb FIFO instance
 * @param[in] reqb : the mreqb to be put
 *
 * @return : void
 */
void mreqb_fifo_put(mreqb_fifo_t _fifo, struct mreqb *reqb)
{
	struct mreqb_fifo *fifo = _fifo;

	cyg_mutex_lock(&fifo->lock);

	list_add_tail(&reqb->node, &fifo->list);

	cyg_mutex_unlock(&fifo->lock);

	cyg_flag_setbits(&fifo->wakeup, 0x01);
}

/**
 * @brief get a mreqb from the FIFO
 *
 * @param[in] _fifo : the mreqb FIFO instance
 *
 * @return : a mreqb pointer, if fifo empty, then NULL return.
 */
struct mreqb* mreqb_fifo_get(mreqb_fifo_t _fifo)
{
	struct mreqb_fifo *fifo = _fifo;
	struct mreqb *reqb = NULL;

	cyg_mutex_lock(&fifo->lock);

	if (!list_empty(&fifo->list)) {
		reqb = list_entry(fifo->list.next, struct mreqb, node);
		list_del(&reqb->node);
		INIT_LIST_HEAD(&reqb->node);
	}

	cyg_mutex_unlock(&fifo->lock);

	return reqb;
}


/**
 * @brief get a mreqb from the FIFO, if fifo empty, will wait util fifo available
 *
 * @param[in] _fifo : the mreqb FIFO instance
 *
 * @return : a mreqb pointer
 */
struct mreqb* mreqb_fifo_waited_get(mreqb_fifo_t _fifo)
{
	struct mreqb_fifo *fifo = _fifo;
	struct mreqb *reqb;

_start:
	cyg_mutex_lock(&fifo->lock);

	if (list_empty(&fifo->list)) {

		cyg_mutex_unlock(&fifo->lock);

		cyg_flag_wait(&fifo->wakeup, 0x1, CYG_FLAG_WAITMODE_OR | CYG_FLAG_WAITMODE_CLR);

		goto _start;
	}

	reqb = list_entry(fifo->list.next, struct mreqb, node);
	list_del(&reqb->node);
	INIT_LIST_HEAD(&reqb->node);

	cyg_mutex_unlock(&fifo->lock);

	return reqb;
}
