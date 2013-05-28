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

static int mf_debug_level = MFLOG_INFO;

void mf_set_debug_level(int level)
{
	if (level >= MFLOG_VERBOSE && level <= MFLOG_CRITICAL)
		mf_debug_level = level;
}


void __mf_log_print(int level, const char* tag, const char* fmt, ...)
{
	va_list ap;

	if (level >= mf_debug_level) {
		va_start(ap, fmt);

		if (tag != NULL) {
#ifdef LOG_OUT_MAILBOX
			mf_LogPrintf("%s: ", tag);
#else
			diag_printf("%s: ", tag);
#endif
		}

#ifdef LOG_OUT_MAILBOX
		mf_LogvPrintf(fmt, ap);
#else
		diag_vprintf(fmt, ap);
#endif
		va_end(ap);
	}

}


