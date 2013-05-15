#ifndef _MREQB_RESERVE_POOL_H
#define _MREQB_RESERVE_POOL_H

typedef void * mreqb_reservepool_t;

externC mreqb_reservepool_t mreqb_reservepool_create(int prealloc_num, int extra_data_size);
externC void mreqb_reservepool_free(mreqb_reservepool_t _pool, struct mreqb* mreq);
externC struct mreqb* mreqb_reservepool_alloc(mreqb_reservepool_t _pool);
externC void mreqb_reservepool_free(mreqb_reservepool_t _pool, struct mreqb* mreq);

#endif	/* _MREQB_RESERVE_POOL_H */
