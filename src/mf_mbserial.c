/**
 *
 * Serial Driver base on MICPROTO in MiFi Project
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		virtual serial driver
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *		wangxiwei <wangxiwei@innofidei.com>
 *
 * @date:	2013-04-09
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>

#include <micproto.h>
#include <micp_cmd.h>
#include <mf_thread.h>
#include <mf_mbserial.h>
#include <mreqb_reservep.h>
#include "mreqb_fifo.h"

#define LOG_TAG		"mf_mbserial"
#include <mf_debug.h>

enum MBSERIAL_SUBCMD {
    MBSERIAL_CMD_STARTUP  = 0,
    MBSERIAL_CMD_SHUTDOWN,
    MBSERIAL_CMD_DATA_TRANSFER,
};

struct mbserial_port {
	int		line;
	int		status;
#define PORT_STATUS_UP		0x01

	cyg_mutex_t	lock;

	struct mf_notifier_head notifier;
};

static struct mbserial_port	*allports[MAX_MBSERIAL_NUM];
static mreqb_reservepool_t mreqb_pool;

static mreqb_fifo_t	pending;

#define PRE_ALLOC_MREQ_NUM			(32)
#define PRE_ALLOC_DATA_SIZE			(512)

static void put_mreqb(struct mreqb *mreq)
{
	mreqb_reservepool_free(mreqb_pool, mreq);
}

static inline struct mreqb *get_mreqb(void)
{
	return mreqb_reservepool_alloc(mreqb_pool);
}


static int handle_mbserial_request(struct mreqb *reqb)
{
	int cmd, line;
	unsigned char *buf = NULL;
	int len;
	struct mbserial_port *port;

	cmd = reqb->subcmd;
	line = MREQB_GET_ARG(reqb, 0);

	if (line >= MAX_MBSERIAL_NUM) {
		MFLOGE("%s receive invaild type %d\n", __FUNCTION__, line);
		return -1;
	}

	port = allports[line];

	switch (cmd) {
	case MBSERIAL_CMD_STARTUP:
		port->status |= PORT_STATUS_UP;

		mf_notifier_call_chain(&port->notifier, MBSERIAL_EVENT_STARTUP, NULL, 0);

		return 0;

	case MBSERIAL_CMD_SHUTDOWN:
		port->status &= ~PORT_STATUS_UP;

		mf_notifier_call_chain(&port->notifier, MBSERIAL_EVENT_SHUTDOWN, NULL, 0);

		return 0;

	case MBSERIAL_CMD_DATA_TRANSFER:
		break;

	default:
		MFLOGE("%s receive invaild command %d\n", __FUNCTION__, cmd);
		return -1;
	}

	len = MREQB_GET_ARG(reqb, 1);
	buf = reqb->extra_data;
	buf[len] = '\0';		/* make sure end with '\0' */

	mf_notifier_call_chain(&port->notifier, MBSERIAL_EVENT_DATAVAIL, buf, len);

	return 0;
}


/*
 * mailbox serial dispatch thread
 */
static void mbserial_dispatch_thread(void *priv)
{
	int ret;
	struct mreqb *reqb;

	do {
		reqb = mreqb_fifo_waited_get(pending);

		ret = handle_mbserial_request(reqb);

		/* complete mreqb */
		mreqb_giveback(reqb, ret);

	} while (1);

}

/*
 * Mailbox SERIAL_REQUEST Command handle function, called by MICPROTO
 */
static int do_mbserial_request(struct mreqb *reqb, void *priv)
{
	mreqb_fifo_put(pending, reqb);

	/* defer to thread to handle it */
	return MREQB_RET_PENDING;
}

/**
 * @brief mailbox serial transfer function
 *
 * @param[in] ser: mailbox serial channel.
 * @param[in] data: data to transfer.
 * @param[in] len: lenth of the data.
 *
 * @return 	actual lenth to tranfer.
 */
int mf_mbserial_send(mbserial_t ser, void *data, int len)
{
	struct mbserial_port *mp = ser;
	struct mreqb *request;
	int tail_len;
	int done_bytes;
	const unsigned char* psrc = data;


	if (len <= 0 || NULL == data) {
		return 0;
	}

	if (!(mp->status & PORT_STATUS_UP)) {
		// there is no user in remote endpoint
		return 0;
	}

	cyg_mutex_lock(&mp->lock);

	tail_len = len;
	done_bytes = 0;

	while (tail_len > 0) {
		int tmp_len;

		request = get_mreqb();

		if (NULL == request) {
			break;
		}

		tmp_len = MIN(PRE_ALLOC_DATA_SIZE, tail_len);
		memcpy(request->extra_data, (void*)(psrc + done_bytes), tmp_len);


		MREQB_BIND_CMD(request, SERIAL_REQUEST);
		MREQB_SET_SUBCMD(request, MBSERIAL_CMD_DATA_TRANSFER);

		MREQB_PUSH_ARG(request, mp->line);
		MREQB_PUSH_ARG(request, tmp_len);

		request->complete = put_mreqb;

		mreqb_submit(request);

		done_bytes += tmp_len;
		tail_len -= tmp_len;
	}

	cyg_mutex_unlock(&mp->lock);

	return done_bytes;
}

/**
 * @brief get a virtual serial channel, and also register a rx notifier callback
 *
 * @param[in] line : virtual serial channel number
 * @param[in] cb : receive notifier function
 *
 * @return if success return a valid channel, otherwise return MBSERIAL_INVALID.
 */
mbserial_t mf_mbserial_get(int line, struct mf_notifier_block *nb)
{
	struct mbserial_port *mp;

	if (line >= MAX_MBSERIAL_NUM) {
		return MBSERIAL_INVALID;
	}

	mp = allports[line];

	if (mp == NULL) {
		mp = (struct mbserial_port *)malloc(sizeof(*mp));

		if (mp == NULL) {
			return MBSERIAL_INVALID;
		}

		memset((void *)mp, 0, sizeof(*mp));

		mp->line = line;
		cyg_mutex_init(&mp->lock);
		INIT_MF_NOTIFIER_HEAD(&mp->notifier);

		allports[line] = mp;
	}

	if (nb != NULL) {
		mf_notifier_chain_register(&mp->notifier, nb);
	}

	return (mbserial_t)mp;
}

/**
 * @brief add a notifier to the mailbox serial channel
 *
 * @param[in] ser : mailbox serial channel
 * @param[in] nb : notifier block to be added
 *
 * @return : if success return 0
 */
int mf_mbserial_register_notifier(mbserial_t ser, struct mf_notifier_block *nb)
{
	struct mbserial_port *mp = ser;

	return mf_notifier_chain_register(&mp->notifier, nb);
}

/**
 * @brief unregister the notifier from the mailbox serial channel
 *
 * @param[in] ser : mailbox serial channel
 * @param[in] nb : notifier block to be removed
 *
 * @return : if success return 0
 */
int mf_mbserial_unregister_notifier(mbserial_t ser, struct mf_notifier_block *nb)
{
	struct mbserial_port *mp = ser;

	return mf_notifier_chain_unregister(&mp->notifier, nb);
}



MF_SYSINIT int mf_mbserial_init(mf_gd_t *gd)
{
	pending = mreqb_fifo_create();

	if (pending == (mreqb_fifo_t)NULL) {
		MFLOGE("mbnand create mreqb FIFO failed!\n");
		return -1;
	}

	mreqb_pool = mreqb_reservepool_create(PRE_ALLOC_MREQ_NUM, PRE_ALLOC_DATA_SIZE);

	mf_thread_create_and_runEx(mbserial_dispatch_thread,
	                           NULL,
	                           "mailbox serial",
	                           CYGNUM_HAL_STACK_SIZE_TYPICAL * sizeof(long),
	                           10);

	mreqb_register_cmd_handler(C_SERIAL_REQUEST, do_mbserial_request, NULL);

	return 0;
}
