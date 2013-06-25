/**
 * Log Interface based on mailbox serial channel
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		log output system
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2013-04-10
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/kernel/kapi.h>
#include <mf_comn.h>
#include <mf_mbserial.h>
#include <mf_thread.h>
#include <libmf.h>

/* TODO : when can print, free log buffer to save memory */

#define LOG_BUFSZ	(4096)

static char log_buffer[LOG_BUFSZ];

static char *log_buf = log_buffer;

static mbserial_t channel;
static cyg_mutex_t lock;
static int can_print;

#define LOG_BUF_MASK		(LOG_BUFSZ - 1)
#define LOG_BUF(idx)		(log_buf[(idx) & LOG_BUF_MASK])
static unsigned log_start;	/* index to log_buf: next char to be read */
static unsigned log_end;	/* index to log_buf: most-recently-written-char + 1 */

static void flush_log_buf(void)
{
	unsigned start, end, boundary;

	for (;;) {
		cyg_mutex_lock(&lock);
		if (log_start == log_end) {
			cyg_mutex_unlock(&lock);
			//nothing to print
			break;
		}

		start = log_start;
		end = log_end;
		log_start = log_end;	/* flush */

		/* 
		 * the ranage between start and end may cross the buffer boundary, so find
		 * the boundary firstly, and then output two part buffer
		 */
		boundary = _ALIGN_UP(start, LOG_BUFSZ);

		if (boundary < end) {
			mf_mbserial_send(channel, (void *)(&LOG_BUF(start)), boundary - start);
			mf_mbserial_send(channel, (void *)(&LOG_BUF(boundary)), end - boundary);

		} else {
			mf_mbserial_send(channel, (void *)(&LOG_BUF(start)), end - start);
		}

		cyg_mutex_unlock(&lock);
	}
}


static void emit_log_char(char c)
{
	LOG_BUF(log_end) = c;
	log_end++;

	if (log_end - log_start > LOG_BUFSZ)
		log_start = log_end - LOG_BUFSZ;
}

static int cache_log_buf(const char *buf, int len)
{
	int i;

	cyg_mutex_lock(&lock);

	for (i=0; i<len; i++) {
		emit_log_char(buf[i]);
	}

	cyg_mutex_unlock(&lock);

	return len;
}

/**
 * @brief output log buffer
 *
 * @param[in] buf : log buffer
 * @param[in] len : log buffer length
 *
 * @return : actual length sent
 */
MF_API int mf_LogSend(const char* buf, int len)
{
	int ret = 0;

	if (can_print) {
		ret = mf_mbserial_send(channel, (void*)buf, len);

	} else {
		ret = cache_log_buf((const char*)buf, len);
	}

	return ret;
}

/**
 * @brief format output
 *
 * @param[in] fmt : format string, like printf().
 *
 * @return : void
 */
MF_API void mf_LogPrintf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	mf_LogvPrintf(fmt, ap);
	va_end(ap);
}

/**
 * @brief format output
 *
 * @param[in] fmt : format string, like vprintf().
 *
 * @return : void
 */
MF_API void mf_LogvPrintf(const char *fmt, va_list ap)
{
	int n;
	char buf[128];

	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	mf_LogSend(buf, n);
}

static int log_mbserial_notifier(struct mf_notifier_block *nb, unsigned long action, void *data, int len)
{
	switch (action) {
	case MBSERIAL_EVENT_STARTUP:
		flush_log_buf();

		can_print = 1;
		break;

	case MBSERIAL_EVENT_SHUTDOWN:
		can_print = 0;
		break;

	default:
		break;
	}

	return 0;
}

static struct mf_notifier_block log_notifier = {
	.notifier_call = log_mbserial_notifier,
};

MF_SYSINIT int mf_log_init(mf_gd_t *gd)
{
	cyg_mutex_init(&lock);

	channel = mf_mbserial_get(gd->log_mbser_line, &log_notifier);
	if (channel == MBSERIAL_INVALID)
		return -1;

	return 0;
}

