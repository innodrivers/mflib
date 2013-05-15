/*
 * micproto.c
 *
 * Mailbox Interface Communication Protocol between CPUs
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
#include <mf_mailbox.h>
#include <mf_cache.h>
#include <mf_debug.h>
#include <micproto.h>


#define MICP_VER		"1.0.1"

#define LOCAL_TRACE		0

#define WORD_SIZE				sizeof(unsigned long)
#define WORD_ALIGN_UP(x)		_ALIGN_UP(x, WORD_SIZE)
#define WORD_ALIGN_DOWN(x)		_ALIGN_DOWN(x, WORD_SIZE)
#define DWORD_SIZE				(WORD_SIZE << 1)

#define msec_to_tick(msec)  ((cyg_tick_count_t)(msec+9)/10)
static void process_output(void);



#define USE_ECOS_MEMPOOL
#define USE_THREAD_POOL
#define MAX_CONCURRENT_HANDLES		(4)

#define CFG_UCLINUX


#ifdef USE_THREAD_POOL

#ifdef CFG_UCLINUX
#define THREAD_POOL_HANDLE_RESPONSE
#else
#define THREAD_POOL_HANDLE_REQUEST
#endif

#endif

//===========================================================================
//                               DATA TYPES
//===========================================================================
typedef struct {
	cyg_sem_t		wakeup;
	mf_thread_t		thread_data;
	cyg_bool		running;

	void*			data;
}pool_entry_t;

struct sync_mreqb_context {
	cyg_flag_t	done;
	int			status;
};

//===========================================================================
//                              LOCAL DATA
//===========================================================================
#if CYG_HAL_LTE_ARM2_ENABLE
#define INPUT_THREAD_PRIO		3
#define OUTPUT_THREAD_PRIO		2
#define POOL_THREAD_PRIO		3
#else
#define INPUT_THREAD_PRIO		4
#define OUTPUT_THREAD_PRIO		4
#define POOL_THREAD_PRIO		4

#ifdef CFG_UCLINUX
#undef POOL_THREAD_PRIO
#define POOL_THREAD_PRIO		16		// interact with uClinux, the pool thread used to handle response.
#endif

#endif

struct micp_statistics {
//	unsigned long alloc_count;		/* mreqb alloc count */
//	unsigned long free_count;		/* mreqb free count */

	unsigned long submit_count;		/* how many mreqb be submit */
	unsigned long complete_count;	/* how many mreqb have been completed (got response) */

	unsigned long receive_count;	/* receive remote request count */
	unsigned long giveback_count;	/* how many request have been giveback (sent response) */
};

static struct {
	mf_mbox_t		mbox;
	mf_thread_t      otd;	// output thread data
	mf_thread_t      itd;	// input thread data

#ifdef USE_THREAD_POOL
	pool_entry_t		pool_threads[MAX_CONCURRENT_HANDLES];
#endif

	struct list_head output_pending;
	cyg_mutex_t output_lock;		// protect output_pending link-list
	cyg_flag_t output_flag;

	struct list_head input_pending;
	cyg_mutex_t input_lock;		// protect input_pending link-list

	struct micp_statistics	statis;
}the_proto;


#undef MICP_SCALLM_CMD
#undef MICP_MCALLS_CMD
#include <micp_cmd.h>

#if CYG_HAL_LTE_ARM2_ENABLE
#define MICP_MCALLS_CMD(symbol, func)		\
	[C_##symbol] = {func},

#define MICP_SCALLM_CMD(symbol, func)		\
	[C_##symbol] = {0},
	
#else
#define MICP_SCALLM_CMD(symbol, func)		\
	[C_##symbol] = {func},

#define MICP_MCALLS_CMD(symbol, func)		\
	[C_##symbol] = {0},

#endif

#define MICP_BOTHCALL_CMD(symbol, func)		\
	[C_##symbol] = {func},

static struct cmd_entry{
	cmd_func_t func;
	void *data;
}micp_cmd_info[MICP_CMD_COUNT] = {
	[C_UNKNOWN] = { 0 },
#include <micp_cmd.h>	
};
#undef MICP_SCALLM_CMD
#undef MICP_MCALLS_CMD
#undef MICP_BOTHCALL_CMD

int mreqb_register_cmd_handler(int cmd, cmd_func_t func, void* data)
{
	if (cmd >= 0 && cmd < MICP_CMD_COUNT) {
		micp_cmd_info[cmd].func = func;
		micp_cmd_info[cmd].data = data;
		return 0;
	}

	return -1;
}

//===========================================================================
//                        INLINE FUNCTIONS
//===========================================================================
/*
static inline void inc_mreqb_alloc_count(void)
{
	the_proto.statis.alloc_count++;
}

static inline void inc_mreqb_free_count(void)
{
	the_proto.statis.free_count++;
}
*/

static inline void inc_mreqb_submit_count(void)
{
	the_proto.statis.submit_count++;
}

static inline void inc_mreqb_complete_count(void)
{
	the_proto.statis.complete_count++;
}

static inline void inc_mreqb_receive_count(void)
{
	the_proto.statis.receive_count++;
}

static inline void inc_mreqb_giveback_count(void)
{
	the_proto.statis.giveback_count++;
}


int mreqb_submit(struct mreqb* mreqb)
{
	cyg_mutex_lock(&the_proto.output_lock);

	if (!mreqb_is_response(mreqb)) {
		inc_mreqb_submit_count();
		list_add_tail(&mreqb->node, &the_proto.output_pending);

	} else {
		/* response mreqb insert into the beginning of list */
		list_add(&mreqb->node, &the_proto.output_pending);
	}

	cyg_mutex_unlock(&the_proto.output_lock);
	
	if (1) {
		//wakeup output thread
		cyg_flag_setbits(&the_proto.output_flag, 0x1);
	} else {
		process_output();
	}

	return 0;
}

static void mreqb_blocking_completion(struct mreqb* mreqb)
{
	struct sync_mreqb_context *ctx = mreqb->context;

	ctx->status = mreqb->result;
	cyg_flag_setbits(&ctx->done, 0x1);
}

void mreqb_completion_free_mreqb(struct mreqb* mreqb)
{
	mreqb_free(mreqb);
}

int mreqb_submit_and_wait(struct mreqb* mreqb, int timeout)
{
	struct sync_mreqb_context ctx;
	int ret;
	unsigned long expire;
	
	cyg_flag_init(&ctx.done);
	
	mreqb->complete = mreqb_blocking_completion;
	mreqb->context = &ctx;

	ret = mreqb_submit(mreqb);
	
   if (timeout) {
        expire = msec_to_tick(timeout);
        ret = cyg_flag_timed_wait(&ctx.done, 0x1, CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR, cyg_current_time() + expire);
    } else {
        ret = cyg_flag_wait(&ctx.done, 0x1, CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
    }
    
    if (ret == 0) {
    	ret = -ETIMEDOUT;
    } else {
    	ret = ctx.status;
    }
    
    return ret;
}

int mreqb_giveback(struct mreqb* mreqb, int status)
{
	mreqb->result = status;
	mreqb->cmd |= RESPONSE_BIT;
	
	inc_mreqb_giveback_count();

	return mreqb_submit(mreqb);
}

static inline void list_replace(struct list_head *old, struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void list_del_init(struct list_head* entry)
{
	list_del(entry);
	INIT_LIST_HEAD(entry);
}

static void process_output(void)
{
	static struct mreqb* pending = NULL;
	struct mreqb* mreqb = NULL;
	struct mreqb* p;
	struct list_head temp_head;

_start:
	while (pending) {
		struct mreqb* next = NULL;

		if (!list_empty(&pending->node)) {
			next = list_entry(pending->node.next, struct mreqb, node);
			list_del_init(&pending->node);
		}

		if (mf_mbox_msg_put(the_proto.mbox, (unsigned long)pending) == 0)  {
			pending = next;
		} else {
			return;
		}
	}

	cyg_mutex_lock(&the_proto.output_lock);
	if (list_empty(&the_proto.output_pending)) {
		cyg_mutex_unlock(&the_proto.output_lock);
		return;
	}

	INIT_LIST_HEAD(&temp_head);

	// change list head
	list_replace(&the_proto.output_pending, &temp_head);
	INIT_LIST_HEAD(&the_proto.output_pending);

	cyg_mutex_unlock(&the_proto.output_lock);


	/* clean cache if need */
	list_for_each_entry(p, &temp_head, node) {

		if (p->cache_update.num > 0) {
			int i;
			for (i=0; i<p->cache_update.num; i++) {
				mf_cache_clean_range((void*)p->cache_update.range[i].start, p->cache_update.range[i].len);
				LTRACEF("clean cache range : [%08x, %08x)\n", 
							p->cache_update.range[i].start,  p->cache_update.range[i].start + p->cache_update.range[i].len);
			}
		}

	}

	mreqb = list_entry(temp_head.next, struct mreqb, node);
	list_del(&temp_head);

	//send request block via mailbox
	pending = mreqb;
	goto _start;

}


static void output_thread(void* priv)
{
	while (1) {
		struct mreqb* mreqb = NULL;
		struct mreqb* p;
		struct list_head temp_head;

_wakeup:
		cyg_mutex_lock(&the_proto.output_lock);
		if (list_empty(&the_proto.output_pending)) {
			LTRACEF("no pending request, to sleep!\n");
			cyg_mutex_unlock(&the_proto.output_lock);

			// wait new request added
			cyg_flag_wait(&the_proto.output_flag, 0x1, CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
			LTRACEF("wake up!\n");
			goto _wakeup;
		}
		
		INIT_LIST_HEAD(&temp_head);

		// change list head
		list_replace(&the_proto.output_pending, &temp_head);
		INIT_LIST_HEAD(&the_proto.output_pending);

		cyg_mutex_unlock(&the_proto.output_lock);

		/* clean cache if need */
		list_for_each_entry(p, &temp_head, node) {

			if (p->cache_update.num > 0) {
				int i;
				for (i=0; i<p->cache_update.num; i++) {
					mf_cache_clean_range((void*)p->cache_update.range[i].start, p->cache_update.range[i].len);
					LTRACEF("clean cache range : [%08x, %08x)\n", 
								p->cache_update.range[i].start,  p->cache_update.range[i].start + p->cache_update.range[i].len);
				}
			}

		}

		mreqb = list_entry(temp_head.next, struct mreqb, node);
		list_del(&temp_head);

		while (mreqb) {
			struct mreqb *next = NULL;

			if (!list_empty(&mreqb->node)) {
				next = list_entry(mreqb->node.next, struct mreqb, node);
				list_del_init(&mreqb->node);
			}

__re_push:
			if (mf_mbox_msg_put(the_proto.mbox, (unsigned long)mreqb) == 0) {
				mreqb = next;
			} else {
				cyg_thread_delay(1);
				goto __re_push;
			}
		}
	}

}

/* handle the mreqb response */
static int process_complete(struct mreqb* mreqb)
{
	if (mreqb->complete != NULL) {

		// invalid cache if need
		if (mreqb->cache_update.num > 0) {
			int i;
			for (i=0; i<mreqb->cache_update.num; i++) {
				mf_cache_inv_range((void*)mreqb->cache_update.range[i].start, mreqb->cache_update.range[i].len);
			}
		}

		mreqb->complete(mreqb);
	}

	return 0;
}

static int process_request(struct mreqb* mreqb)
{
	int ret = -1;
	struct cmd_entry *handler;
	int i;
	
	LTRACEF("cmd = %d\n", mreqb->cmd);

	if (mreqb->cmd >= MICP_CMD_COUNT) {
		goto error_nohandler;
	} 

	handler = &micp_cmd_info[mreqb->cmd];

	if (handler->func) {

		//invalid cache if need
		if (mreqb->cache_update.num > 0) {
			for (i=0; i<mreqb->cache_update.num; i++) {
				mf_cache_inv_range((void*)mreqb->cache_update.range[i].start, mreqb->cache_update.range[i].len);
			}
		}
		
		ret = handler->func(mreqb, handler->data);

		if (MREQB_RET_PENDING != ret)
			mreqb_giveback(mreqb, ret);

	} else {
		goto error_nohandler;
	}

	return 0;
	
error_nohandler:
	mreqb_giveback(mreqb, MREQB_RET_NOHANDLE);

	return ret;
}

#ifdef USE_THREAD_POOL
static void pool_thread_function(void *priv)
{
	pool_entry_t *pool_entry = (pool_entry_t*)priv;

	while (1) {
		struct mreqb* mreqb = NULL;
		cyg_semaphore_wait(&(pool_entry->wakeup));

		mreqb = (struct mreqb*)pool_entry->data;
		
__re_enter:
		if (mreqb) {
#ifdef THREAD_POOL_HANDLE_REQUEST
			process_request(mreqb);
#else
			process_complete(mreqb);
#endif


			// check if there is pending request need to handle, if so pick one.
			cyg_mutex_lock(&the_proto.input_lock);
			if (!list_empty(&the_proto.input_pending)) {
				mreqb = list_entry(the_proto.input_pending.next, struct mreqb, node);
				list_del_init(&mreqb->node);
				cyg_mutex_unlock(&the_proto.input_lock);
				goto __re_enter;
				
			}
			cyg_mutex_unlock(&the_proto.input_lock);
		}

		pool_entry->running = false;
	}

}

static void pool_threads_initialize(void)
{
	int i;

	for (i=0; i<MAX_CONCURRENT_HANDLES; i++) {
		pool_entry_t *entry = &the_proto.pool_threads[i]; 
		char thread_name[16];

		cyg_semaphore_init(&entry->wakeup, 0);
		entry->running = false;
		entry->data = NULL;

		snprintf(thread_name, 16, "worker%d", i);
		entry->thread_data = mf_thread_createEx(pool_thread_function,
									(void*)entry,
									thread_name,
									CYGNUM_HAL_STACK_SIZE_TYPICAL * sizeof(long),
									POOL_THREAD_PRIO);
		mf_thread_run(entry->thread_data);
	}
}

static int dispatch_pool_thread_run(struct mreqb* mreqb)
{
	int i;

	for (i=0; i<MAX_CONCURRENT_HANDLES; i++) {
		pool_entry_t *entry = &the_proto.pool_threads[i]; 

		if (entry->running)
			continue;
		
		entry->data = (void*)mreqb;
		entry->running = true;
		LTRACEF("wakeup %d worker\n", i);
		cyg_semaphore_post(&(entry->wakeup));

		return 0;
	}

	// add to pending list
	cyg_mutex_lock(&the_proto.input_lock);
	list_add_tail(&mreqb->node, &the_proto.input_pending);
	cyg_mutex_unlock(&the_proto.input_lock);

	return -1;
}
#endif	// USE_THREAD_POOL

static void input_thread(void* priv)
{
	while (1) {
		struct mreqb *mreqb;
		
		//get mreqb from mailbox
		mreqb = (struct mreqb*)mf_mbox_msg_get(the_proto.mbox);
		LTRACEF("got message %08lx\n", (unsigned long)mreqb);

		if (mreqb->magic != MREQB_MAGIC) {
			dprintf(ERROR, "Receive Invalid Message!\n");
			continue;
		}

		if (mreqb_is_response(mreqb)) {
			LTRACEF("recved request is a response, to complete it!\n");

			inc_mreqb_complete_count();

#ifdef THREAD_POOL_HANDLE_RESPONSE
			dispatch_pool_thread_run(mreqb);
#else
			process_complete(mreqb);
#endif
			
			
		} else {
			inc_mreqb_receive_count();

#ifdef THREAD_POOL_HANDLE_REQUEST
			dispatch_pool_thread_run(mreqb);
#else
			LTRACEF("process request in input_thread\n");	
			process_request(mreqb);
#endif
		}

	}

}


void dump_micproto_statistics(void)
{
	dprintf(ALWAYS, "micproto statistics:\n");
//	dprintf(ALWAYS, "\tmreqb alloc %ld, free %ld\n", the_proto.statis.alloc_count, the_proto.statis.free_count);
	dprintf(ALWAYS, "\tmreqb submit %ld, complete %ld\n", the_proto.statis.submit_count, the_proto.statis.complete_count);
	dprintf(ALWAYS, "\tmreqb recv %ld, giveback %ld\n", the_proto.statis.receive_count, the_proto.statis.giveback_count);
}

MF_SYSINIT int mf_micproto_init(mf_gd_t *gd)
{
	dprintf(ALWAYS, "Mailbox Protocol, v%s\n", MICP_VER);

	the_proto.mbox = mf_mbox_get("CPU2");
	if (MF_MBOX_INVALID == the_proto.mbox)
		return -1;

	INIT_LIST_HEAD(&the_proto.output_pending);
	cyg_mutex_init(&the_proto.output_lock);
	
	cyg_flag_init(&the_proto.output_flag);

	INIT_LIST_HEAD(&the_proto.input_pending);
	cyg_mutex_init(&the_proto.input_lock);

#ifdef USE_THREAD_POOL
	pool_threads_initialize();
#endif
	the_proto.otd = mf_thread_createEx(output_thread,
										NULL,
										"mailbox output",
										CYGNUM_HAL_STACK_SIZE_TYPICAL * sizeof(long), 
										OUTPUT_THREAD_PRIO);
						
	the_proto.itd = mf_thread_createEx(input_thread,
										NULL,
										"mailbox input",
										CYGNUM_HAL_STACK_SIZE_TYPICAL * sizeof(long), 
										INPUT_THREAD_PRIO);

	//start thread						
	mf_thread_run(the_proto.otd);
	mf_thread_run(the_proto.itd);

	return 0;
}
