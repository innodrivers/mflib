/**
 *
 * USB simulated WAN interface
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		USB based WAN interface
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *
 * @date:	2013-04-23
 *
 */
#define MFLOG_TAG	"usbwan"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/cyg_type.h>
#include <linux/list.h>
#include <mf_comn.h>
#include <mf_thread.h>
#include <mf_cache.h>

#include <mf_nbuf.h>

#include <nbuf_queue.h>
#include <libmf.h>

#include "usbnet.h"

#include <mf_debug.h>

#define ETH_HLEN    14


static mf_nbuf_queue_t *backlog;	/* nbuf list going to be process */

#define RESERVED_DATA_LEN		(32)

extern int mf_NetifPacketXmitAsync(struct mf_nbuf *nbuf);


/* forward the packet to the CPU2 router */
static void usbwan_forward_thread(void *priv)
{
	struct mf_nbuf *nbuf;

	while (1) {
		nbuf = mf_nbuf_queue_waited_get(backlog);

		/* remove the ether frame header, forward ip packet to CPU2 */
		mf_netbuf_pull(nbuf, ETH_HLEN);

		mf_NetifPacketXmitAsync(nbuf);

		mf_netbuf_free(nbuf);
	}

}

/* receive packet from usb net */
static void usbwan_rx_thread(void *priv)
{
	struct mf_nbuf *nbuf;
	int len;

	usbnet_init();

	cyg_thread_delay(1000);

	mf_NetifLinkupInd();

	while (1) {
		nbuf = mf_netbuf_alloc();
		if (nbuf == NULL) {
			MFLOGW("no more netbuf for rx\n");
			cyg_thread_delay(20);
			continue;
		}

		mf_netbuf_reserve(nbuf, RESERVED_DATA_LEN);
_requeue:
		len = usbnet_queue_rx(nbuf->data, mf_netbuf_tailroom(nbuf));
		if (len <= 0) {
			MFLOGE("invalid rx packet! (len=%d)", len);
			goto _requeue;
		}

		mf_netbuf_put(nbuf, len);

		mf_nbuf_queue_put(backlog, nbuf);
	}
}

MF_CALLBACK int mf_NetifPacketRxCallback(void* ip_packet, int packet_len)
{
	usbnet_queue_tx(ip_packet, packet_len);

	return 0;
}

/*----------------------------------------------------------*/

int netif_usbwan_init(void)
{
	backlog = mf_nbuf_queue_create();
	if (backlog == NULL) {
		return -1;
	}

	mf_thread_create_and_run(usbwan_rx_thread, NULL);
	mf_thread_create_and_run(usbwan_forward_thread, NULL);

	return 0;
}
