/*
 * mf_netbuf.h
 *
 *  Created on: Apr 25, 2013
 *      Author: drivers
 */

#ifndef MF_NETBUF_H_
#define MF_NETBUF_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <mf_comn.h>

struct mf_nbuf;

typedef void (*mf_netbuf_free_callback_fn)(struct mf_nbuf *);

typedef struct mf_nbuf {
	struct mf_nbuf *next;

	unsigned char *head;	/* buffer head pointer, init when allocate, don't modify it */
	unsigned char *end;		/* buffer end pointer, init when allocate, don't modify it */
	unsigned char *data;
	unsigned char *tail;
	int   len;				/* payload length */

	int ref;				/* reference count */

	void *priv;
	mf_netbuf_free_callback_fn free_cb;

} mf_nbuf_t;

#define MF_NETBUF_SIZE		_ALIGN_UP(sizeof(struct mf_nbuf), 32)
#define MF_PACKETBUF_SIZE	(2048 - MF_NETBUF_SIZE)

static inline void mf_netbuf_reset_tail_pointer(struct mf_nbuf *nb)
{
	nb->tail = nb->data;
}

static inline void mf_netbuf_reserve(struct mf_nbuf* nb, int len)
{
	nb->data += len;
	nb->tail += len;
}

static inline int mf_netbuf_tailroom(struct mf_nbuf* nb)
{
	return nb->end - nb->tail;
}

static inline void mf_netbuf_put(struct mf_nbuf *nb, int len)
{
	nb->tail += len;
	nb->len += len;
}


extern struct mf_nbuf* mf_netbuf_alloc(void);
extern void mf_netbuf_ref(struct mf_nbuf* p);
extern void mf_netbuf_free(struct mf_nbuf *p);

#ifdef __cplusplus
}
#endif
#endif /* MF_NETBUF_H_ */
