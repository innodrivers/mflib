/**
 *
 * Net Interface Driver base on MICPROTO in MiFi Project
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		virtual net interface driver
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2013-04-19
 *
 */
#define LOG_TAG		"mf_mbnetif"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/infra/cyg_type.h>
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>

#include <libmf.h>
#include <mf_comn.h>
#include <micproto.h>
#include <micp_cmd.h>
#include <mf_thread.h>
#include <mf_nbuf.h>
#include <nbuf_queue.h>
#include <mreqb_reservep.h>
#include "mreqb_fifo.h"

#include <mf_debug.h>

//#define RX_BUF_LATE_FREE

#define PREALLOC_MREQB_NUM		128

enum {
    MBNET_CMD_LINKUP_IND = 0,
    MBNET_CMD_LINKDOWN_IND,
    MBNET_CMD_IP_PACKET_NEW,
    MBNET_CMD_IP_PACKET_FREE,
};

#define MBNET_RET_LATE_FREE		(0x444c4fa8)		// ascii "HOLD"

static mreqb_reservepool_t		req_pool;



static mf_nbuf_queue_t *forward_backlog;	/* to be forward */

#ifdef RX_BUF_LATE_FREE
#define	FREEPKTID_BUF_NUM	64
static u32 free_pktid_buf[FREEPKTID_BUF_NUM];

#define FREEPKTID_BUF_MASK		(FREEPKTID_BUF_NUM - 1)
#define FREEPKTID_BUF(idx)		(free_pktid_buf[(idx) & FREEPKTID_BUF_MASK])

static unsigned free_pktid_start;	/* index to free_pktid_buf: next pktid to be read */
static unsigned free_pktid_end;	/* index to free_pktid_buf: most-recently-written-pktid + 1 */
#endif

/*------------------------------------------------------------*/


/*------------------------------------------------------------*/
static mf_netif_rx_func_t netif_rx_cb;

/**
 * @brief register a packet receive callback function
 *
 * @param[in] cb : callback function to register
 *
 * @return : if success return 0, otherwise return a negative error code.
 *
 */
MF_API int mf_NetifPacketRxCallbackRegister(mf_netif_rx_func_t cb)
{
	netif_rx_cb = cb;

	return 0;
}

/**
 * @brief ip packet receive
 *
 * @param[in] ip_packet : the received ip packet buffer
 * @param[in] packet_len : the ip packet length
 *
 * @return : if success return 0, otherwise return a negative error code.
 *
 */
MF_CALLBACK __attribute__((weak)) int mf_NetifPacketRx(void *ip_packet, int packet_len)
{
	if (netif_rx_cb != NULL)
		return netif_rx_cb(ip_packet, packet_len);

	return 0;
}

__attribute__((weak)) int mf_netif_rx(struct mf_nbuf *nbuf)
{
	int ret;

	ret = mf_NetifPacketRx(nbuf->data, nbuf->len);

	mf_netbuf_free(nbuf);

	return ret;
}

static void recycle_mreqb_resource(struct mreqb *reqb)
{
	mreqb_reservepool_free(req_pool, reqb);
}


/**
 * @brief notify LTE net available
 *
 * @return : if success return 0, otherwise return -1.
 */
MF_API int mf_NetifLinkupInd(void)
{
	struct mreqb *reqb;

	reqb = mreqb_reservepool_alloc(req_pool);
	if (reqb == NULL) {
		MFLOGE("netif linkup notify failed (no mreqb)\n");
		return -1;
	}

	MREQB_BIND_CMD(reqb, NET_REQUEST);
	MREQB_SET_SUBCMD(reqb, MBNET_CMD_LINKUP_IND);

	reqb->complete = recycle_mreqb_resource;

	return mreqb_submit(reqb);
}

/**
 * @brief notify LTE net unavailable
 *
 * @return : if success return 0, otherwise return -1.
 */
MF_API int mf_NetifLinkdownInd(void)
{
	struct mreqb *reqb;

	reqb = mreqb_reservepool_alloc(req_pool);
	if (reqb == NULL) {
		MFLOGE("netif linkdown notify failed (no mreqb)\n");
		return -1;
	}

	MREQB_BIND_CMD(reqb, NET_REQUEST);
	MREQB_SET_SUBCMD(reqb, MBNET_CMD_LINKDOWN_IND);

	reqb->complete = recycle_mreqb_resource;

	return mreqb_submit(reqb);
}

#define __BUILD_NET_REQUEST_MREQB(__rq, __buf, __len)	\
	do {	\
		MREQB_BIND_CMD(__rq, NET_REQUEST);	\
		MREQB_SET_SUBCMD(__rq, MBNET_CMD_IP_PACKET_NEW);	\
		MREQB_PUSH_ARG(__rq, __buf); 	/* use as packet id */	\
		MREQB_PUSH_ARG(__rq, __buf);	\
		MREQB_PUSH_ARG(__rq, __len);	\
		MREQB_PUSH_CACHE_UPDATE(__rq, __buf, __len);	\
	} while (0)

/**
 * @brief LTE downstream packet transmit
 *
 * @param[in] ip_packet : IP packet buffer
 * @param[in] packet_len : IP packet buffer length
 *
 * @return if success return 0, otherwise return -1.
 */
MF_API int mf_NetifPacketXmit(void *ip_packet, int packet_len)
{
	struct mreqb *reqb;
	int ret;

	reqb = mreqb_reservepool_alloc(req_pool);
	if (reqb == NULL) {
		MFLOGE("packet tx failed (no mreqb)\n");
		return -1;
	}

	__BUILD_NET_REQUEST_MREQB(reqb, ip_packet, packet_len);

	ret = mreqb_submit_and_wait(reqb, 0);

	mreqb_reservepool_free(req_pool, reqb);

	return ret;
}

static void xmit_async_complete(struct mreqb* reqb)
{
	struct mf_nbuf *nbuf = reqb->context;

	mreqb_reservepool_free(req_pool, reqb);

	mf_netbuf_free(nbuf);
}

MF_API int mf_NetifPacketXmitAsync(struct mf_nbuf *nbuf)
{
	struct mreqb *reqb;
	int ret;

	// increase reference to prevent being free
	mf_netbuf_ref(nbuf);

	reqb = mreqb_reservepool_alloc(req_pool);
	if (reqb == NULL) {
		MFLOGE("packet tx failed (no mreqb)\n");
		return -1;
	}

	__BUILD_NET_REQUEST_MREQB(reqb, nbuf->data, nbuf->len);

	reqb->context = nbuf;

	reqb->complete = xmit_async_complete;

	// no block
	ret = mreqb_submit(reqb);

	return ret;
}

#ifdef RX_BUF_LATE_FREE

/* if success, return how many pktid sent */
static int send_packet_free_cmd(u32 pktid[], int num)
{
	struct mreqb *reqb;
	int i;
	int count;

	reqb = mreqb_reservepool_alloc(req_pool);
	if (reqb == NULL) {
		MFLOGE("no mreqb when free ip packet\n");
		return -1;
	}

	MREQB_BIND_CMD(reqb, NET_REQUEST);
	MREQB_SET_SUBCMD(reqb, MBNET_CMD_IP_PACKET_FREE);

	count = MAX(num , 8);		// 8 : mreqb max argument number
	for (i=0; i<count; i++) {
		MREQB_PUSH_ARG(reqb, pktid[i]);
	}

	reqb->complete = recycle_mreqb_resource;

	mreqb_submit(reqb);

	return count;
}


/* This function will be when mf_netbuf going to be free */
static void rx_netbuf_free_cb(struct mf_nbuf *nbuf)
{
	int ret;
	u32 pktid = (u32)nbuf->priv;

	if (pktid == 0)
		return;

	/* TODO : has risk of lost pktid to be free */

	FREEPKTID_BUF(free_pktid_end) = pktid;
	free_pktid_end++;

	if (free_pktid_end - free_pktid_start > FREEPKTID_BUF_NUM)
		free_pktid_start = free_pktid_end - FREEPKTID_BUF_NUM;

	while ((free_pktid_end - free_pktid_start) >= 8) {
		ret = send_packet_free_cmd(&FREEPKTID_BUF(free_pktid_start),
								(free_pktid_end - free_pktid_start));
		if (ret > 0) {
			free_pktid_start += ret;
			continue;
		}

		break;
	}

}


#endif	/*  RX_BUF_LATE_FREE */

static void netbuf_forward_thread(void *priv)
{
	struct mf_nbuf *nbuf;

	do {
		nbuf = mf_nbuf_queue_waited_get(forward_backlog);

		mf_netif_rx(nbuf);

	} while (1);
}



static int handle_new_ip_packet(struct mreqb *reqb)
{
	struct mf_nbuf *nbuf;
	u32 pktid;
	void *ip_pkt;
	int pkt_len;
	int ret = 0;

	nbuf = mf_netbuf_alloc();

	if (nbuf != NULL) {
		pktid = (u32) MREQB_GET_ARG(reqb, 0);
		ip_pkt = (void *) MREQB_GET_ARG(reqb, 1);
		pkt_len = (int) MREQB_GET_ARG(reqb, 2);

#ifdef RX_BUF_LATE_FREE
		/* use CPU2's buffer */
		nbuf->data = ip_pkt;
		nbuf->head = nbuf->data;
		mf_netbuf_reset_tail_pointer(nbuf);
		mf_netbuf_put(nbuf, pkt_len);
		nbuf->end = nbuf->tail;

		nbuf->priv = (void*) pktid;
		nbuf->free_cb = rx_netbuf_free_cb;

		ret = MBNET_RET_LATE_FREE;

#else
		/* copy packet into our buffer */
		memcpy(nbuf->data, ip_pkt, pkt_len);
		mf_netbuf_put(nbuf, pkt_len);

#endif

		mf_nbuf_queue_put(forward_backlog, nbuf);

	} else {
		MFLOGE("packet rx failed (no nbuf)\n");
	}

	return ret;
}


/*
 * Mailbox NET_REQUEST Command handle function, called by MICPROTO
 */
static int do_net_request(struct mreqb *reqb, void *priv)
{
	/* we only handle IP_PACKET_NEW command, other command should
	 * never be received.
	 */
	if (reqb->subcmd == MBNET_CMD_IP_PACKET_NEW) {
		return handle_new_ip_packet(reqb);
	}

	return 0;
}


MF_SYSINIT int mf_netif_init(mf_gd_t *gd)
{
	req_pool = mreqb_reservepool_create(PREALLOC_MREQB_NUM, 0);

	forward_backlog = mf_nbuf_queue_create();

	mf_thread_create_and_runEx(netbuf_forward_thread,
	                           NULL,
	                           "mbnetif forward",
	                           CYGNUM_HAL_STACK_SIZE_TYPICAL * sizeof(long),
	                           6);

	mreqb_register_cmd_handler(C_NET_REQUEST, do_net_request, NULL);

	return 0;
}
