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
#include <mf_debug.h>
#include <mf_nbuf.h>

#include <nbuf_queue.h>
#include <libmf.h>

#include "usbnet.h"

#define ETH_HLEN    14

#define NETBUF_NUM		(256)


static mf_nbuf_queue_t *freebufs;		/* nbuf list can be allocated */
static mf_nbuf_queue_t *process_backlog;	/* nbuf list going to be process */

#define RESERVED_DATA_LEN		(0)



/* forward the packet to the CPU2 router */
static void usbwan_forward_thread(void *priv)
{
	struct mf_nbuf *nbuf;
	unsigned char* ethbuf;
	unsigned char* ipbuf;

	while (1) {
		nbuf = mf_nbuf_queue_waited_get(process_backlog);

		ethbuf = nbuf->data;
		ipbuf = ethbuf + ETH_HLEN;

		/* remove the ether frame header, forward ip packet to CPU2 */
		mf_NetifPacketXmit(ipbuf, nbuf->len - ETH_HLEN);
	}

}

/* receive packet from usb net */
static void usbwan_rx_thread(void *priv)
{
	struct mf_nbuf *nbuf;
	int len;

	while (1) {
		nbuf = mf_nbuf_queue_waited_get(freebufs);

		mf_netbuf_reserve(nbuf, RESERVED_DATA_LEN);
		len = usbnet_queue_rx(nbuf->data, mf_netbuf_tailroom(nbuf));
		mf_netbuf_put(nbuf, len);
		
		mf_nbuf_queue_put(process_backlog, nbuf);
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
	int i;

	usbnet_init();

	freebufs = mf_nbuf_queue_create();
	process_backlog = mf_nbuf_queue_create();
	if (freebufs == NULL || process_backlog == NULL) {
		return -1;
	}

	for (i=0; i<NETBUF_NUM; i++) {
		struct mf_nbuf *nbuf = mf_netbuf_alloc();
		if (nbuf == NULL)
			break;

		mf_nbuf_queue_put(freebufs, nbuf);
	}

	mf_thread_create_and_run(usbwan_rx_thread, NULL);
	mf_thread_create_and_run(usbwan_forward_thread, NULL);

	return 0;
}
