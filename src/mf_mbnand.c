/**
 *
 * Nand Proxy Driver base on MICPROTO in MiFi Project
 *
 * Copyright 2012 Innofidei Inc.
 *
 * @file
 * @brief
 *		nand proxy driver
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2012-12-28
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
#include <micp_cmd.h>
#include <mf_mbnand.h>
#include <libmf.h>

#include "mreqb_fifo.h"

#define LOG_TAG		"mf_mbnand"
#include <mf_debug.h>

static mreqb_fifo_t	nreq_fifo;


static int handle_nand_readid(struct mreqb *rq)
{
	struct mbnand_readid_arg *arg;
	int ret;

	arg = rq->extra_data;

	ret = mf_NandReadIDCallback(arg->id, sizeof(arg->id), (int *)&arg->len);

	return ret;
}

static int handle_nand_getinfo(struct mreqb *rq)
{
	struct mbnand_getinfo_arg *arg;
	int ret;

	arg = rq->extra_data;

	ret = mf_NandReadIDCallback(arg->id, sizeof(arg->id), (int *)&arg->id_len);

	if (ret) {
		return ret;
	}

	ret = mf_NandGetInfoCallback(&arg->pagesize, &arg->blocksize, &arg->chipsize, &arg->iowidth16);

	if (ret) {
		return ret;
	}

	return 0;
}

static int handle_nand_chipselect(struct mreqb *rq)
{
	struct mbnand_chipselect_arg *arg;
	int ret;

	arg = rq->extra_data;

	ret = mf_NandChipSelectCallback(arg->cs);

	return ret;
}

static int handle_nand_readpage(struct mreqb *rq)
{
	struct mbnand_readpage_arg *arg;
	int ret;

	arg = rq->extra_data;

	ret = mf_NandReadPageCallback(arg->page, arg->column, arg->buf, arg->len, (int *)&arg->retlen);

	return ret;
}

static int handle_nand_writepage(struct mreqb *rq)
{
	struct mbnand_writepage_arg *arg;
	int ret;

	arg = rq->extra_data;
	arg->status = 0;

	ret = mf_NandWritePageCallback(arg->page, arg->column, arg->buf, arg->len, (int *)&arg->retlen);

	if (ret) {
		arg->status = MB_NAND_STATUS_FAIL;
	}

	return ret;
}

static int handle_nand_eraseblock(struct mreqb *rq)
{
	struct mbnand_eraseblock_arg *arg;
	int ret;

	arg = rq->extra_data;
	arg->status = 0;

	ret = mf_NandEraseBlockCallback(arg->page);

	if (ret) {
		arg->status = MB_NAND_STATUS_FAIL;
	}

	return ret;
}

static int handle_nand_request(struct mreqb *rq)
{
	int reqid = rq->subcmd;
	int ret;

	switch (reqid) {
	case MB_NAND_READID:
		ret = handle_nand_readid(rq);
		break;

	case MB_NAND_GETINFO:
		ret = handle_nand_getinfo(rq);
		break;

	case MB_NAND_CHIPSELECT:
		ret = handle_nand_chipselect(rq);
		break;

	case MB_NAND_READPAGE:
		ret = handle_nand_readpage(rq);
		break;

	case MB_NAND_WRITEPAGE:
		ret = handle_nand_writepage(rq);
		break;

	case MB_NAND_ERASEBLOCK:
		ret = handle_nand_eraseblock(rq);
		break;

	default:
		MFLOGE("unkwown request id (%d) !\n", reqid);
		return -MB_NAND_RET_NOTSUPPORT;
	}

	return ret;
}


static void mbnand_request_handle_thread(void *priv)
{
	int ret;
	struct mreqb *rq;

	do {
		rq = mreqb_fifo_waited_get(nreq_fifo);

		ret = handle_nand_request(rq);

		/* complete mreqb */
		mreqb_giveback(rq, ret);

	} while (1);
}

/*
 * Mailbox NAND_REQUEST Command handle function, called by MICPROTO
 */
static int do_mbnand_request(struct mreqb *rq, void *data)
{
	mreqb_fifo_put(nreq_fifo, rq);

	return MREQB_RET_PENDING;	/* defer to thread to handle it */
}

MF_SYSINIT int mf_mbnand_init(mf_gd_t *gd)
{
	nreq_fifo = mreqb_fifo_create();

	if (nreq_fifo == (mreqb_fifo_t)NULL) {
		MFLOGE("mbnand create mreqb FIFO failed!\n");
		return -1;
	}

	mf_thread_create_and_runEx(mbnand_request_handle_thread,
	                           NULL,
	                           "mailbox nand",
	                           CYGNUM_HAL_STACK_SIZE_TYPICAL * sizeof(long),
	                           7);

	mreqb_register_cmd_handler(C_NAND_REQUEST, do_mbnand_request, NULL);

	return 0;
}

