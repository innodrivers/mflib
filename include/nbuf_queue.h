/*
 * nbuf_queue.h
 *
 *  Created on: Apr 26, 2013
 *      Author: drivers
 */

#ifndef NBUF_QUEUE_H_
#define NBUF_QUEUE_H_
#ifdef __cplusplus
extern "C" {
#endif

typedef struct mf_nbuf_queue {
	struct mf_nbuf *head;
	struct mf_nbuf **last;

	int			elem_num;
	cyg_flag_t		event;
} mf_nbuf_queue_t;


extern mf_nbuf_queue_t * mf_nbuf_queue_create(void);
extern void mf_nbuf_queue_put(mf_nbuf_queue_t *q, struct mf_nbuf *nbuf);
extern struct mf_nbuf * mf_nbuf_queue_get(mf_nbuf_queue_t *q);
extern struct mf_nbuf * mf_nbuf_queue_waited_get(mf_nbuf_queue_t *q);

#ifdef __cplusplus
}
#endif
#endif /* NBUF_QUEUE_H_ */
