#ifndef _MF_DEBUG_H
#define _MF_DEBUG_H

#ifndef MFLOG_TAG
#define MFLOG_TAG		NULL
#endif

#ifndef MFLOG_NDEBUG
#ifdef NDEBUG
#define MFLOG_NDEBUG	1
#else
#define MFLOG_NDEBUG	0
#endif
#endif

/* debug level */
enum {
	MFLOG_VERBOSE,
	MFLOG_DEBUG,
	MFLOG_INFO,
	MFLOG_WARNING,
	MFLOG_ERROR,
	MFLOG_CRITICAL,
};

#if MFLOG_NDEBUG
#define MFLOGV(...)		((void)0)
#else
#define MFLOGV(...)		((void)(MFLOG(MFLOG_VERBOSE, MFLOG_TAG, __VA_ARGS__)))
#endif

#define MFLOGD(...)		((void)(MFLOG(MFLOG_DEBUG, MFLOG_TAG, __VA_ARGS__)))
#define MFLOGI(...)		((void)(MFLOG(MFLOG_INFO, MFLOG_TAG, __VA_ARGS__)))
#define MFLOGW(...)		((void)(MFLOG(MFLOG_WARNING, MFLOG_TAG, __VA_ARGS__)))
#define MFLOGE(...)		((void)(MFLOG(MFLOG_ERROR, MFLOG_TAG, __VA_ARGS__)))
#define MFLOGC(...)		((void)(MFLOG(MFLOG_CRITICAL, MFLOG_TAG, __VA_ARGS__)))

#define MFLOG(level, tag, ...)		__mf_log_print(level, tag, __VA_ARGS__)

#define mf_printf(level, ...)	__mf_log_print(level, NULL, __VA_ARGS__)

extern void __mf_log_print(int level, const char* tag, const char* fmt, ...);



#endif	/* _MF_DEBUG_H */
