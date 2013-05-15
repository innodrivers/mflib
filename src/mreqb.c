/*
 * mailbox request block management
 *
 * copyright 2012 innofidei inc.
 *
 * Author(s):  jimmy.li <lizhengming@innofidei.com>
 * Date:         2012-05-15
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/kernel/kapi.h>
#include <linux/list.h>

#include <mf_comn.h>
#include <mf_thread.h>
#include <micproto.h>

#define MFLOG_TAG	"mreqb"
#include <mf_debug.h>

#define WORD_SIZE				sizeof(unsigned long)
#define WORD_ALIGN_UP(x)		_ALIGN_UP(x, WORD_SIZE)
#define WORD_ALIGN_DOWN(x)		_ALIGN_DOWN(x, WORD_SIZE)
#define DWORD_SIZE				(WORD_SIZE << 1)


#define USE_ECOS_MEMPOOL


//===========================================================================
//                        EXTRA-DATA of MREQB ALLOCATION/FREE
//===========================================================================

#ifdef USE_ECOS_MEMPOOL

typedef struct edata_pool {
	cyg_handle_t mem_handle;
	cyg_mempool_var mem_val;
} edata_pool_t;

static edata_pool_t *edata_pool;

static edata_pool_t* edata_pool_create(void *base, unsigned long len)
{
	edata_pool_t *pool;

	pool = (edata_pool_t *)malloc(sizeof(*pool));
	if (pool == NULL)
		return NULL;

	cyg_mempool_var_create(base, len, &pool->mem_handle, &pool->mem_val);

	return pool;
}

static void* edata_pool_waited_alloc(edata_pool_t *pool, unsigned long size)
{
	// This waits if the memory is not  currently available.
	return cyg_mempool_var_alloc(pool->mem_handle, size);
}

static void edata_pool_free(edata_pool_t *pool, void *p)
{
	cyg_mempool_var_free(pool->mem_handle, p);
}

#else

typedef struct edata_pool {
	void* base;
	u32		len;
	struct list_head free_list;

	cyg_mutex_t lock;
	cyg_flag_t 	wait;
} edata_pool_t;

static edata_pool_t *edata_pool;

struct mem_chunk {
	u32   prev_size;
	u32   curr_size;

	struct list_head node;
};

#define LIST_NODE_SIZE             WORD_ALIGN_UP(sizeof(struct list_head))
#define LIST_NODE_ALIGN(nSize)     (((nSize) + LIST_NODE_SIZE - 1) & ~(LIST_NODE_SIZE - 1))
#define IS_FREE(nSize)             (((nSize) & (WORD_SIZE - 1)) == 0)
#define GET_CHUNK_SIZE(chunk)          ((chunk)->curr_size & ~(WORD_SIZE - 1))
#define GET_PREVCHUNK_SIZE(chunk)          ((chunk)->prev_size & ~(WORD_SIZE - 1))

static inline struct mem_chunk *get_successor_chunk(struct mem_chunk *p)
{
	return (struct mem_chunk *)((unsigned long)p + DWORD_SIZE + GET_CHUNK_SIZE(p));
}

static inline struct mem_chunk *get_predeccessor_chunk(struct mem_chunk *p)
{
	return (struct mem_chunk *)((unsigned long)p - GET_PREVCHUNK_SIZE(p) - DWORD_SIZE);
}


static inline void set_chunk_size(struct mem_chunk *p, unsigned long size)
{
	struct mem_chunk *succ;

	p->curr_size = size;

	succ = get_successor_chunk(p);
	succ->prev_size = size;
}

static edata_pool_t* edata_pool_create(void *base, unsigned long len)
{
	edata_pool_t* pool;
	unsigned long start, end;
	struct mem_chunk *pFirst, *pTail;
	
	MFLOGD("nocached buffer range : [%08lx, %08lx)\n", (unsigned long)base, (unsigned long)base + len);

	pool = (edata_pool_t*)malloc(sizeof(*pool));
	if (pool == NULL)
		return NULL;

	pool->base = base;
	pool->len = len;
	INIT_LIST_HEAD(&pool->free_list);
	cyg_mutex_init(&pool->lock);
	cyg_flag_init(&pool->wait);

	cyg_mutex_lock(&pool->lock);
	start = WORD_ALIGN_UP((unsigned long)base);
	end   = WORD_ALIGN_DOWN((unsigned long)base + len);

	pFirst = (struct mem_chunk *)start;
	pTail  = (struct mem_chunk *)(end - DWORD_SIZE);

	pFirst->prev_size = 1;
	pFirst->curr_size = (unsigned long)pTail - (unsigned long)pFirst - DWORD_SIZE;

	pTail->prev_size = pFirst->curr_size;
	pTail->curr_size = 1;

	list_add_tail(&pFirst->node, &pool->free_list);
	cyg_mutex_unlock(&pool->lock);

	return 0;
}

static void* edata_pool_alloc(edata_pool_t *pool, unsigned long size)
{
	void *p = NULL;
	unsigned long alloc_size, rest_size;
	struct mem_chunk *curr, *succ;

	cyg_mutex_lock(&pool->lock);
	alloc_size = LIST_NODE_ALIGN(size);
	list_for_each_entry(curr, &pool->free_list, node) {
		if (curr->curr_size >= alloc_size)
			goto do_alloc;
	}
	cyg_mutex_unlock(&pool->lock);

	return NULL;

do_alloc:
	list_del(&curr->node);

	rest_size = curr->curr_size - alloc_size;

	if (rest_size < sizeof(struct mem_chunk)) {
		set_chunk_size(curr, curr->curr_size | 1);
	}	else {
		set_chunk_size(curr, alloc_size | 1);

		succ = get_successor_chunk(curr);
		set_chunk_size(succ, rest_size - DWORD_SIZE);
		
		list_add_tail(&succ->node, &pool->free_list);
	}

	p = &curr->node;
	cyg_mutex_unlock(&pool->lock);

	return p;	
}

static void edata_pool_free(edata_pool_t *pool, void *p)
{
	struct mem_chunk *curr, *succ;

	cyg_mutex_lock(&pool->lock);

	curr = (struct mem_chunk *)((unsigned long)p - DWORD_SIZE);
	succ = get_successor_chunk(curr);

	if (IS_FREE(succ->curr_size)) {
		set_chunk_size(curr, GET_CHUNK_SIZE(curr) + succ->curr_size + DWORD_SIZE);
		list_del(&succ->node);
		
	}	else {
		set_chunk_size(curr, GET_CHUNK_SIZE(curr));
	}

	if (IS_FREE(curr->prev_size)){
		struct mem_chunk *prev;

		prev = get_predeccessor_chunk(curr);
		set_chunk_size(prev, prev->curr_size + curr->curr_size + DWORD_SIZE);
		
	}	else {
		list_add_tail(&curr->node, &pool->free_list);
	}

	cyg_flag_setbits(&pool->wait, 0x1);

	cyg_mutex_unlock(&pool->lock);

}


static void* edata_pool_waited_alloc(edata_pool_t *pool, unsigned long size)
{
	void * data;

	while (1) {
		data = edata_pool_alloc(pool, size);
		if (data == NULL) {
			cyg_flag_wait(&pool->wait, 0x1, CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
			continue;
		}
		break;
	}

	return data;
}
#endif


//===========================================================================
//                          MREQB ALLOCATION/FREE
//===========================================================================

struct mreqb_memnode {
	struct mreqb_memnode *next;
};

typedef struct mreqb_pool {
	struct mreqb_memnode* free;
	cyg_mutex_t		lock;
	cyg_flag_t 		event;
} mreqb_pool_t;

static mreqb_pool_t	 *mreqb_pool;

static mreqb_pool_t* mreqb_pool_create(void *base, unsigned long len)
{
	mreqb_pool_t *pool;
	u8* mreqb_memnode_memory;
	u16 mreqb_memnode_num;
	struct mreqb_memnode *m, *chunk;
	u16 size;
	int i;

	pool = (mreqb_pool_t *)malloc(sizeof(*pool));
	if (pool == NULL)
		return NULL;

	cyg_mutex_init(&pool->lock);
	cyg_flag_init(&pool->event);

	size = WORD_ALIGN_UP(sizeof(struct mreqb) + sizeof(struct mreqb_memnode));
	mreqb_memnode_memory = (u8*)base;
	mreqb_memnode_num = len / size;

	cyg_mutex_lock(&pool->lock);

	chunk = (struct mreqb_memnode*)mreqb_memnode_memory;
	pool->free = chunk;
	m = chunk;

	for (i=0; i<mreqb_memnode_num; i++) {
		m->next = (struct mreqb_memnode*)((u8*)m + size);
		chunk = m;
		m = m->next;
	}
	chunk->next = NULL;
	cyg_mutex_unlock(&pool->lock);

	return pool;
}


static struct mreqb* __mreqb_pool_alloc(mreqb_pool_t* pool)
{
	struct mreqb_memnode* m;
	void *mem = NULL;

	m = pool->free;
	if (m != NULL) {
		pool->free= m->next;
		m->next = NULL;

		mem = (u8*)m + sizeof(struct mreqb_memnode);
	}

	return (struct mreqb*)mem;
}

static void __mreqb_pool_free(mreqb_pool_t* pool, struct mreqb* mreqb)
{
	struct mreqb_memnode *m;

	m = (struct mreqb_memnode*)((u8*)mreqb - sizeof(struct mreqb_memnode));

	m->next = pool->free;
	pool->free = m;

	if (m->next == NULL) {
		cyg_flag_setbits(&pool->event, 0x1);
	}
}

static struct mreqb* mreqb_pool_alloc(mreqb_pool_t *pool)
{
	struct mreqb *rq;

	cyg_mutex_lock(&pool->lock);

	rq = __mreqb_pool_alloc(pool);

	cyg_mutex_unlock(&pool->lock);

	return rq;
}

static struct mreqb* mreqb_pool_waited_alloc(mreqb_pool_t *pool)
{
	struct mreqb *rq;

	while(1) {
		rq = mreqb_pool_alloc(pool);
		if (rq == NULL) {
			cyg_flag_wait(&pool->event, 0x1, CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
			continue;
		}
		break;
	}

	return rq;
}

static void mreqb_pool_free(mreqb_pool_t* pool, struct mreqb* mreqb)
{
	cyg_mutex_lock(&pool->lock);

	__mreqb_pool_free(pool, mreqb);

	cyg_mutex_unlock(&pool->lock);
}

/**
 * @breif free the mailbox request block
 *
 * @param[in] mreqb :
 */
void mreqb_free(struct mreqb* mreqb)
{
	if (mreqb->extra_data_size > 0 && mreqb->extra_data != NULL) {
		edata_pool_free(edata_pool, mreqb->extra_data);
		mreqb->extra_data = NULL;
		mreqb->extra_data_size = 0;
	}

	mreqb_pool_free(mreqb_pool, mreqb);
}

/**
 * @brief alloc a mailbox request block
 *
 * @param[in] extra_data_size : the uncached data size
 *
 * @return : pointer to a valid mreqb.
 *
 * @note : this function may block when memory not available
 */
struct mreqb* mreqb_waited_alloc(int extra_data_size)
{
	struct mreqb* req;

	req = mreqb_pool_waited_alloc(mreqb_pool);

	memset((void*)req, 0, sizeof(struct mreqb));

	if (extra_data_size > 0) {
		req->extra_data = edata_pool_waited_alloc(edata_pool, extra_data_size);
		req->extra_data_size = extra_data_size;
	}

	req->magic = MREQB_MAGIC;
	INIT_LIST_HEAD(&req->node);

	return req;
}


/**
 * @brief reuse the mailbox request block
 *
 * @param[in] mreqb : the mreqb to be re-use
 *
 * @return : void
 */
void mreqb_reinit(struct mreqb* mreqb)
{
	void *extra_data = mreqb->extra_data;
	unsigned int extra_data_size = mreqb->extra_data_size;

	memset((void*)mreqb, 0, sizeof(struct mreqb));

	mreqb->extra_data = extra_data;
	mreqb->extra_data_size = extra_data_size;

	mreqb->magic = MREQB_MAGIC;
	INIT_LIST_HEAD(&mreqb->node);
}

void mreqb_dump(struct mreqb* mreqb)
{
	mf_printf(MFLOG_INFO, "-------dump mreqb %08lx ------\n", (unsigned long)mreqb);
	mf_printf(MFLOG_INFO, "magic : %08x\n", mreqb->magic);
	mf_printf(MFLOG_INFO, "cmd : %08x\n", mreqb->cmd);
	mf_printf(MFLOG_INFO, "subcmd : %08x\n", mreqb->subcmd);
	mf_printf(MFLOG_INFO, "argc : %d\n", mreqb->argc);
	mf_printf(MFLOG_INFO, "prio : %d\n", mreqb->prio);
	mf_printf(MFLOG_INFO, "context : %08lx\n", (unsigned long)mreqb->context);
	mf_printf(MFLOG_INFO, "complete : %08lx\n", (unsigned long)mreqb->complete);
	mf_printf(MFLOG_INFO, "\n");
}

MF_SYSINIT int mf_mreqb_init(mf_gd_t *gd)
{
	void *mreqb_pool_base, *extra_data_pool_base;
	int mreqb_pool_size, extra_data_pool_size;

	extra_data_pool_base = (void*)gd->mbox_reserve_addr;
	extra_data_pool_size = (gd->mbox_reserve_size * 3)/4;

	mreqb_pool_base= (void*)((int)extra_data_pool_base + extra_data_pool_size);
	mreqb_pool_size= gd->mbox_reserve_size/4;

	edata_pool = edata_pool_create(extra_data_pool_base, extra_data_pool_size);

	mreqb_pool = mreqb_pool_create(mreqb_pool_base, mreqb_pool_size);

	return 0;
}
