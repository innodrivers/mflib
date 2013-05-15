/**
 *
 * mreqb reserve pool management
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		mreqb reserve pool
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *
 * @date:	2013-04-09
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/kernel/kapi.h>

#include <micproto.h>
#include <mreqb_reservep.h>

struct mreqb_reservep_s {
	int				count;
	int				extra_data_size;
	struct list_head	free;
	cyg_mutex_t			lock;
};

mreqb_reservepool_t mreqb_reservepool_create(int prealloc_num, int extra_data_size)
{
	struct mreqb_reservep_s *pool;
	struct mreqb* mreq;
	int i;

	pool = malloc(sizeof(*pool));
	memset(pool, 0, sizeof(*pool));

	cyg_mutex_init(&pool->lock);
	INIT_LIST_HEAD(&pool->free);


	for (i = 0; i < prealloc_num; i++) {
		mreq = mreqb_alloc(extra_data_size);
		list_add_tail(&mreq->node, &pool->free);
	}

	pool->count = prealloc_num;
	pool->extra_data_size = extra_data_size;

	return (mreqb_reservepool_t)pool;
}

void mreqb_reservepool_destroy(mreqb_reservepool_t _pool)
{
	struct mreqb_reservep_s *pool = _pool;
	struct mreqb* mreq;

	cyg_mutex_lock(&pool->lock);
	
	while (!list_empty(&pool->free)) {
		mreq = list_entry(pool->free.next, struct mreqb, node);
		list_del(&mreq->node);
		mreqb_free(mreq);	
	}
	
	cyg_mutex_unlock(&pool->lock);

	free(pool);
}

void mreqb_reservepool_free(mreqb_reservepool_t _pool, struct mreqb* mreq)
{
	struct mreqb_reservep_s *pool = _pool;

	cyg_mutex_lock(&pool->lock);
	mreqb_reinit(mreq);
	list_add_tail(&mreq->node, &pool->free);
	cyg_mutex_unlock(&pool->lock);
}

struct mreqb* mreqb_reservepool_alloc(mreqb_reservepool_t _pool)
{
	struct mreqb_reservep_s *pool = _pool;
	struct mreqb* mreq = NULL;

	cyg_mutex_lock(&pool->lock);
	
	if (!list_empty(&pool->free)) {
		mreq = list_entry(pool->free.next, struct mreqb, node);
		list_del(&mreq->node);
	}
	
	cyg_mutex_unlock(&pool->lock);

	return mreq;
}
