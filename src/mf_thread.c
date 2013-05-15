/**
 *
 * Thread encapsulate in MiFi Project
 *
 * Copyright 2012 Innofidei Inc.
 *
 * @file
 * @brief
 *		thread create helper
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2012-09-10
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/kernel/kapi.h>
#include <mf_comn.h>
#include <mf_thread.h>

#define LOCAL_TRACE	0


typedef struct {
	cyg_thread   obj;
	cyg_handle_t hdl;
	char		 name[16];

	mf_thread_fn	entry;
	void *pStack;
	unsigned stackSz;

	void *priv;
} thread_data_t;

static void trampoline(cyg_addrword_t param)
{
	thread_data_t *td = (thread_data_t*)param;

	td->entry(td->priv);

	dprintf(ALWAYS, "Thread %s Exiting\n", td->name);

	cyg_thread_exit();

//	free(td);
}

#define MF_STACK_SIZE_MIN	(512)
/**
 * @brief create a thread
 *
 * @param[in] fn	- thread routine function
 * @param[in] priv	- private data of thread routine
 * @param[in] thread_name - thread name
 * @param[in] stack_size - thread stack size in bytes
 * @param[in] priority - the lower value, the higher priority
 *
 * @return - if success return non-zero thread handle, otherwise return NULL.
 */
mf_thread_t mf_thread_createEx(mf_thread_fn fn, void* priv, const char* thread_name, unsigned stack_size, int priority)
{
	thread_data_t *td;
	unsigned totSz;

	if (fn == NULL || stack_size < MF_STACK_SIZE_MIN) {
		return (mf_thread_t)NULL;
	}

	totSz = _ALIGN_UP(sizeof(*td), sizeof(long)) + _ALIGN_UP(stack_size, sizeof(long));

	td = (thread_data_t *)malloc(totSz);
	memset(td, 0, totSz);

	if (thread_name)
		strncpy(td->name, thread_name, sizeof(td->name));

	td->entry = fn;
	td->pStack = (void*)((char*)td + _ALIGN_UP(sizeof(*td), 4));
	td->stackSz = _ALIGN_UP(stack_size, sizeof(long));
	td->priv = priv;

	cyg_thread_create(priority,
	                  trampoline,
	                  (cyg_addrword_t)td,
	                  td->name,
	                  (void *)td->pStack,
	                  td->stackSz,
	                  &td->hdl,
	                  &td->obj);


	return (mf_thread_t)td;
}

/**
 * @brief create a thread
 *
 * @param[in] fn - thread routine function
 * @param[in] priv - private data of thread routine
 *
 * @return - if success return valid thread handle, otherwise return NULL
 */
mf_thread_t mf_thread_create(mf_thread_fn fn, void* priv)
{
	return mf_thread_createEx(fn, priv, "mf_thread", CYGNUM_HAL_STACK_SIZE_TYPICAL * sizeof(long), 10);  
}

/**
 * @brief set a thread into running
 *
 * @param[in] t - the created thread handle
 *
 * @return - none
 */
void mf_thread_run(mf_thread_t t)
{
	thread_data_t *td = (thread_data_t*)t;

	if (td != NULL)
		cyg_thread_resume(td->hdl);
}

/**
 * @brief create a thread and set it running
 *
 * @param[in] fn	- thread routine function
 * @param[in] priv	- private data of thread routine
 * @param[in] thread_name - thread name
 * @param[in] stack_size - thread stack size in bytes
 * @param[in] priority - the lower value, the higher priority
 *
 *
 * @return - if success return non-zero thread handle, otherwise return NULL.
 */
mf_thread_t mf_thread_create_and_runEx(mf_thread_fn fn, void* priv, const char* thread_name, unsigned stack_size, int priority)
{
	mf_thread_t t = mf_thread_createEx(fn, priv, thread_name, stack_size, priority);

	mf_thread_run(t);

	return t;
}

/**
 * @brief create a thread and set it running
 *
 * @param[in] fn	- thread routine function
 * @param[in] priv	- private data of thread routine
 *
 * @return - if success return non-zero thread handle, otherwise return NULL.
 */
mf_thread_t mf_thread_create_and_run(mf_thread_fn fn, void* priv)
{
	mf_thread_t t = mf_thread_create(fn, priv);

	mf_thread_run(t);

	return t;
}

/*------------------- work ------------------*/

static struct mf_workqueue *default_wq;

static void workqueue_thread(void *priv)
{
	struct mf_workqueue *wq = (struct mf_workqueue *)priv;

	while (1) {
		struct mf_work *work;

_restart:
		cyg_mutex_lock(&wq->lock);
		if (list_empty(&wq->list)) {
			cyg_mutex_unlock(&wq->lock);

			cyg_flag_wait(&wq->wakeup, 0x01, CYG_FLAG_WAITMODE_OR|CYG_FLAG_WAITMODE_CLR);
			goto _restart;
		}

		work = list_entry(wq->list.next, struct mf_work, entry);
		list_del(&work->entry);
		cyg_mutex_unlock(&wq->lock);

		if (work && work->func) {
			work->func(work);
		}

	}

}

struct mf_workqueue * mf_create_workqueue(const char* name)
{
	struct mf_workqueue *wq = (struct mf_workqueue *)malloc(sizeof(*wq));

	wq->thread = mf_thread_create(workqueue_thread, wq);
	INIT_LIST_HEAD(&wq->list);
	cyg_mutex_init(&wq->lock);
	cyg_flag_init(&wq->wakeup);
	wq->flag = 0;

	return wq;
}

void mf_destroy_workqueue(struct mf_workqueue *wq)
{
	//TODO
}

int mf_queue_work(struct mf_workqueue *wq, struct mf_work *work)
{
	cyg_mutex_lock(&wq->lock);
	list_add_tail(&work->entry, &wq->list);	
	cyg_mutex_unlock(&wq->lock);

	if (wq->flag & WQ_RUNNING) {
		// wakeup workqueue thread
		cyg_flag_setbits(&wq->wakeup, 0x1);
	} else {
		mf_thread_run(wq->thread);
		wq->flag |= WQ_RUNNING;
	}

	return 0;
}

int mf_schedule_work(struct mf_work *work)
{
	if (default_wq == NULL)
		default_wq = mf_create_workqueue("default");

	return mf_queue_work(default_wq, work);
}

