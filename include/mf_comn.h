#ifndef _MF_COMN_H
#define _MF_COMN_H

#include <cyg/infra/diag.h>
#include <cyg/hal/hal_intr.h>

#undef u32
#undef u16
#undef u8
#undef uint32_t
#undef uint16_t
#undef uint8_t

#define u32		cyg_uint32
#define u16		cyg_uint16
#define u8		cyg_uint8

#define uint32_t	cyg_uint32
#define uint16_t	cyg_uint16
#define uint8_t		cyg_uint8

#define DEBUGLEVEL		4

/* debug levels */
#define CRITICAL	0
#define ALWAYS		0
#define ERROR		1
#define WARNING		2
#define INFO		3
#define VERBOSE		4

#define dprintf(level, x...)		\
	do {							\
		if (level <= DEBUGLEVEL)	\
			diag_printf(x);			\
	} while(0)


#define TRACE_ENTRY 	dprintf(VERBOSE, "%s: entry\n", __FUNCTION__)
#define TRACE_EXIT 		dprintf(VERBOSE, "%s: exit\n", __FUNCTION__)
#define TRACEF(x...)	do { dprintf(VERBOSE, "%s: ", __FUNCTION__); dprintf(VERBOSE, x);} while(0)

/* trace routines that work if LOCAL_TRACE is set */
#define LTRACE_ENTRY do { if (LOCAL_TRACE) { TRACE_ENTRY; } } while (0)
#define LTRACE_EXIT do { if (LOCAL_TRACE) { TRACE_EXIT; } } while (0)
#define LTRACEF(x...) do { if (LOCAL_TRACE) { TRACEF(x); } } while (0)

/* align x on a size boundary - adjust x up/down if needed */
#define _ALIGN_UP(x, size)    (((x)+((size)-1)) & (~((size)-1)))
#define _ALIGN_DOWN(x, size)  ((x) & (~((size)-1)))

#ifndef ARRAY_ZIE
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
#endif

#ifndef MIN
#define MIN(a, b)		(((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b)		(((a) > (b)) ? (a) : (b))
#endif

#ifndef MiB
#define MiB(x)			((x) << 20)
#endif

#ifndef KiB
#define KiB(x)			((x) << 10)
#endif

#define DECL_MF_SYS_PROTECT(oldints)	CYG_INTERRUPT_STATE (oldints)
#define MF_SYS_PROTECT(oldints)			HAL_DISABLE_INTERRUPTS(oldints)
#define MF_SYS_UNPROTECT(oldints)		HAL_RESTORE_INTERRUPTS(oldints)


#define MF_SYSINIT

typedef struct mf_global_data {
	u16		max_mbser_num;		/* mailbox serial support ports */
	u8		at_mbser_line;		/* AT port in mailbox serial */
	u8		log_mbser_line;		/* Log port in mailbox serial */
	u32		cpu2_run_addr;
	u32		mbox_reserve_addr;	/* memory start which reserved for mailbox request */
	u32		mbox_reserve_size;	/* memory size which reserved for mailbox request */
}mf_gd_t;

typedef int (*mf_init_func_t)(mf_gd_t *gd);

#endif	// _MF_COMN_H
