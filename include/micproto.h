/*
 * include/asm-arm/arch-p4a/micproto.h
 *
 *  Copyright (C) 2012 Innofidei Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __ASM_ARCH_P4A_MICPROTO_H
#define __ASM_ARCH_P4A_MICPROTO_H

#include <linux/list.h>
#include <mf_comn.h>

#define MREQB_MAGIC		(0x42514552)
#define MREQB_RET_PENDING	(0x444e4550)
#define MREQB_RET_NOHANDLE	(-0x1000)

struct mreqb;
typedef void (*complete_t)(struct mreqb*);
typedef int (*cmd_func_t)(struct mreqb*, void *);

struct mreqb {
	unsigned int magic;
	unsigned int cmd;
#define RESPONSE_BIT		(1<<31)
	unsigned int subcmd;

	int argc;
	unsigned int argv[8];
	struct {
		int num;
		struct {
			unsigned int start;
			unsigned int len;
		} range[8];
	} cache_update;
	int result;
#ifdef CONFIG_P4A_CPU2
	void *extra_data;
	phys_addr_t extra_data_phys;
#else
	void *extra_data_noused;
	void *extra_data;
#endif
	int extra_data_size;

	int prio; /* range [0, MREQB_MAX_PRIO], the value greater, the priority higher */
	struct list_head node;
	void *context;
	complete_t complete;
};

#define MREQB_MIN_PRIO		0
#define MREQB_MAX_PRIO		3

#define MREQB_BIND_CMD(mreqb, _cmd)	do {mreqb->cmd = C_##_cmd;} while(0)
#define MREQB_SET_SUBCMD(mreqb, _subcmd)	do {mreqb->subcmd = _subcmd;} while(0)
#define MREQB_PUSH_ARG(mreqb, _arg)	do {mreqb->argv[mreqb->argc++] = (unsigned int)_arg;} while(0)

static inline unsigned long MREQB_POP_ARG(struct mreqb *mreqb)
{
	if (mreqb->argc > 0)
		return mreqb->argv[--mreqb->argc];
	return 0;
}

static inline unsigned long MREQB_GET_ARG(struct mreqb *mreqb, int index)
{       
	    return mreqb->argv[index];
}

#define MREQB_EMPTY_ARG(mrqb)	\
	do {		\
		mreqb->argc = 0;	\
	} while (0)

/* NOTICE: for CPU2, the user of mreqb need flush cache manually */
#define MREQB_PUSH_CACHE_UPDATE(mreqb, _start, _len) \
	do { \
		int i = mreqb->cache_update.num; \
		mreqb->cache_update.range[i].start = (unsigned int)_start; \
		mreqb->cache_update.range[i].len = (unsigned int)_len; \
		mreqb->cache_update.num = ++i; \
	} while (0)

#define MREQB_EMPTY_CACHE_UPDATE(mreqb)	\
	do {		\
		mreqb->cache_update.num = 0;	\
	} while (0)

static inline int mreqb_is_response(struct mreqb* mreqb)
{
	return ((mreqb->cmd & RESPONSE_BIT) != 0);
}

extern struct mreqb* mreqb_waited_alloc(int extra_data_size);

static inline struct mreqb *mreqb_alloc(int extra_data_size)
{
	return mreqb_waited_alloc(extra_data_size);
}

extern void mreqb_free(struct mreqb *mreqb);
extern void mreqb_reinit(struct mreqb* mreqb);
extern int mreqb_submit(struct mreqb *mreqb);
extern int mreqb_submit_and_wait(struct mreqb *mreqb, int timeout);
extern int mreqb_giveback(struct mreqb* mreqb, int status);

extern void mreqb_completion_free_mreqb(struct mreqb *mreqb);

extern int mreqb_register_cmd_handler(int cmd, cmd_func_t func, void *data);

#endif
