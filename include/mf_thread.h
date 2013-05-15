#ifndef _MF_THREAD_H
#define _MF_THREAD_H

#include <linux/list.h>

typedef void* mf_thread_t;

typedef void (*mf_thread_fn)(void *priv);

mf_thread_t mf_thread_create(mf_thread_fn fn, void* priv);
mf_thread_t mf_thread_createEx(mf_thread_fn fn, void* priv, const char* thread_name, unsigned stack_size, int priority);

void mf_thread_run(mf_thread_t t);

mf_thread_t mf_thread_create_and_run(mf_thread_fn fn, void* priv);
mf_thread_t mf_thread_create_and_runEx(mf_thread_fn fn, void* priv, const char* thread_name, unsigned stack_size, int priority);


struct mf_work;
typedef void (*work_func_t)(struct mf_work *work);

struct mf_work {
	struct list_head	entry;
	work_func_t			func;
	void *				priv;
};

struct mf_workqueue {
	struct list_head	list;
	cyg_flag_t	wakeup;
	cyg_mutex_t lock;

	unsigned 	flag;
#define WQ_RUNNING		0x01

	mf_thread_t			thread;
};

#define __WORK_INITIALIZER(n, f, p) {              \
	.entry  = { &(n).entry, &(n).entry },           \
	.func = (f),                        \
	.priv = (p),						\
	}

#define MF_DECLARE_WORK(n, f, p)		\
	struct mf_work n = __WORK_INITIALIZER(n, f, p);

#define MF_INIT_WORK(_work, _func, _priv)                 \
    do {                            \
		INIT_LIST_HEAD(&(_work)->entry);		\
		(_work)->func = _func;		\
		(_work)->priv = _priv;		\
	} while (0)

struct mf_workqueue * mf_create_workqueue(const char* name);
void mf_destroy_workqueue(struct mf_workqueue *wq);
int mf_queue_work(struct mf_workqueue *wq, struct mf_work *work);
int mf_schedule_work(struct mf_work *work);


#endif	/* _MF_THREAD_H */
