#ifndef _MREQB_FIFO_H
#define _MREQB_FIFO_H

typedef void* mreqb_fifo_t;

extern mreqb_fifo_t mreqb_fifo_create(void);
extern void mreqb_fifo_put(mreqb_fifo_t fifo, struct mreqb *reqb);
extern struct mreqb* mreqb_fifo_get(mreqb_fifo_t fifo);
extern struct mreqb* mreqb_fifo_waited_get(mreqb_fifo_t fifo);

#endif	/* _MREQB_FIFO_H */
