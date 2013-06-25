/**
 *
 * AT Interface based on mailbox serial channel
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		AT interface
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *
 * @date:	2013-04-19
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/kernel/kapi.h>
#include <mf_comn.h>
#include <mf_mbserial.h>
#include <libmf.h>


static mbserial_t	channel;
static mf_atcmd_rx_func_t	atcmd_rx_cb;

#define ATSEND_BUF_LEN		4096
#define ATRECV_BUF_LEN		1024

static char at_buf[ATSEND_BUF_LEN + ATRECV_BUF_LEN];

static char* atsend_buf = at_buf;
static char* atrecv_buf = at_buf + ATSEND_BUF_LEN;

static int can_send;
static cyg_mutex_t lock;

#define ATSEND_BUF_MASK		(ATSEND_BUF_LEN - 1)
#define ATSEND_BUF(idx)		(atsend_buf[(idx) & ATSEND_BUF_MASK])
static unsigned asb_start;	/* index to atsend_buf: next char to be read */
static unsigned asb_end;	/* index to atsend_buf: most-recently-written-char + 1 */

#define ATRECV_BUF_MASK		(ATRECV_BUF_LEN - 1)
#define ATRECV_BUF(idx)		(atrecv_buf[(idx) & ATRECV_BUF_MASK])
static unsigned arb_end;	/* index to atrecv_buf: most-recently-written-char + 1 */

/**
 * @brief receive AT command
 *
 * @param[in] data : received AT command data
 * @param[in] len : received AT command data length
 *
 * @return : void
 */
MF_CALLBACK __attribute__((weak)) void mf_ATCmdRx(void *data, int len)
{
	if (atcmd_rx_cb != NULL)
		atcmd_rx_cb(data, len);
}


/**
 * @brief register a AT command receive callback function
 *
 * @param[in] cb : callback function to register
 *
 * @return : if success return 0, otherwise return a negative number.
 */
MF_API int mf_ATCmdRxCallbackRegister(mf_atcmd_rx_func_t cb)
{
	atcmd_rx_cb = cb;

	return 0;
}


static void flush_buffered_at(void)
{
	for(;;) {
		unsigned start, end, boundary;

		cyg_mutex_lock(&lock);
		if (asb_start == asb_end) {
			cyg_mutex_unlock(&lock);
			break;
		}

		start = asb_start;
		end = asb_end;
		asb_start = asb_end;
		
		boundary = _ALIGN_UP(start, ATSEND_BUF_LEN);

		if (boundary < end) {
			mf_mbserial_send(channel, (void *)(&ATSEND_BUF(start)), boundary - start);
			mf_mbserial_send(channel, (void *)(&ATSEND_BUF(boundary)), end - boundary);

		} else {
			mf_mbserial_send(channel, (void *)(&ATSEND_BUF(start)), end - start);
		}

		cyg_mutex_unlock(&lock);
	}
}

/* put the AT command into cache buffer for AT channel unavailable currently */
static int cache_atsend_buffer(const char *buf, int len)
{
	int sz;
	const char* psrc;

	sz = MIN(len , ATSEND_BUF_LEN);
	psrc = buf + (len - sz);

	cyg_mutex_lock(&lock);

	while (sz > 0) {
		unsigned end, boundary;
		int space;
		int copy_len;

		end = asb_end;
		boundary = _ALIGN_UP(end+1,  ATSEND_BUF_LEN);

		space = boundary - end;
		copy_len = MIN(space, sz);

		memcpy((void*)(&ATSEND_BUF(end)), psrc, copy_len);

		asb_end += copy_len;

		psrc += copy_len;
		sz -= copy_len;
	}

	if (asb_end - asb_start > ATSEND_BUF_LEN)
		asb_start = asb_end - ATSEND_BUF_LEN;
	
	cyg_mutex_unlock(&lock);

	if (can_send){
		flush_buffered_at();
	}
		
	return len;
}

static char *strnchr(const char *s, size_t count, int c)
{
	for (; count-- && *s != '\0'; ++s) {
		if (*s == (char) c) {
			return (char *) s;
		}
	}

	return NULL ;
}

static void cache_atrecv_buffer(const char *buf, int len)
{
	char *startp;
	char *endp;

	startp = (char*)buf;

	while (len > 0) {
		endp = strnchr(startp,len,'\r');  	//strchr(buf, '\r');
		
		if (endp != NULL) {
			int num = endp - startp + 1;		/* including the '\r' character */
			if (num < len && *(endp+1)=='\n') {
				num++;		/* including the '\n' character */
			}

			//TODO: check if buffer overflow
			memcpy((void*)&atrecv_buf[arb_end], startp, num);
			len -= num;
			arb_end += num;
			startp += num;

			//atrecv_buf[arb_end] = '\0';	/* make sure end with '\0' */
			/* submit the received AT Command to upper layer */
			mf_ATCmdRx(atrecv_buf, arb_end);

			arb_end = 0;

			continue;
		}

		//TODO: check if buffer overflow
		memcpy((void*)&atrecv_buf[arb_end], startp, len);
		arb_end += len;
		len = 0;
	}
}

/**
 * @brief send AT Command string
 *
 * @param[in] data : AT command string
 * @param[in] len : AT command length
 *
 * @return : actual length sent
 */
MF_API int mf_ATSend(const char* data, int len)
{
	int ret = 0;

	if (channel == MBSERIAL_INVALID)
		return 0;

	if (can_send) {
		ret = mf_mbserial_send(channel, (void*)data, len);
	
	} else {
		ret = cache_atsend_buffer(data, len);
	}

	return ret;
}

static int at_mbserial_notifier(struct mf_notifier_block *nb, unsigned long action, void *data, int len)
{
	switch (action) {
	case MBSERIAL_EVENT_DATAVAIL:
		cache_atrecv_buffer(data, len);
		break;
	
	case MBSERIAL_EVENT_STARTUP:
		flush_buffered_at();
		can_send = 1;
		break;

	case MBSERIAL_EVENT_SHUTDOWN:
		can_send = 0;
		break;

	default:
		break;
	}

	return 0;
}

static struct mf_notifier_block at_notifier = {
	.notifier_call = at_mbserial_notifier,
};

MF_SYSINIT int mf_atcmd_init(mf_gd_t *gd)
{
	
	cyg_mutex_init(&lock);
	
	channel = mf_mbserial_get(gd->at_mbser_line, &at_notifier);

	return 0;
}

