/**
 *
 * mifi debug log system
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		debug log interface
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *
 * @date:	2013-04-18
 *
 */
#include <stdio.h>
#include <stdarg.h>
#include <mf_debug.h>
#include <libmf.h>

#include <cyg/infra/diag.h>

int mf_debug_level = MFLOG_INFO;


void __mf_log_print(int level, const char* tag, const char* fmt, ...)
{
	va_list ap;

	if (level >= mf_debug_level) {
		va_start(ap, fmt);

		if (tag != NULL)
			mf_LogSend(tag, strlen(tag) + 1);

#ifdef LOG_OUT_MAILBOX
		mf_LogvPrintf(fmt, ap);
#else
		diag_vprintf(fmt, ap);
#endif
		va_end(ap);
	}

}


