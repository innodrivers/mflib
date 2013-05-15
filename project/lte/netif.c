/**
 *
 * net interface between Mifi and LTE platform.
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		net interface for mifi
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2013-05-08
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libmf.h>

#define LOG_TAG		"netif_adap"
#include <mf_debug.h>

#define netif_report_once()	\
({										\
	static int _report_once = 0;		\
										\
	if (!_report_once) {				\
		_report_once = 1;				\
		MFLOGE("function %s not implement!\n", __FUNCTION__);	\
	}									\
										\
})

__attribute__((weak)) int mf_NetifPacketRxCallback(void *ip_packet, int packet_len)
{
	netif_report_once();
	return -1;
}
