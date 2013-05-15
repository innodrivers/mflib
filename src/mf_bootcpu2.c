/**
 * MiFi Boot CPU2
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		boot cpu2
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2013-04-17
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/kernel/kapi.h>
#include <mf_comn.h>
#include <mf_mbserial.h>
#include <mf_thread.h>
#include <mf_cache.h>
#include <hardware.h>
#include <libmf.h>

/* --------------io operation ----------*/

extern unsigned long cpu2boot_data, cpu2boot_data_end;

static void copy_cpu2_bootcode(unsigned long *dst, unsigned long *src, int num)
{
	int i;
	unsigned long size = num << 2;

	for (i=0; i<num; i++) {
		*dst = *src;
		dst++;
		src++;
	}

	mf_cache_clean_range(dst, size);
}

MF_SYSINIT int mf_boot_cpu2(mf_gd_t *gd)
{
	copy_cpu2_bootcode((unsigned long*)gd->cpu2_run_addr,
						&cpu2boot_data,
						&cpu2boot_data_end - &cpu2boot_data);

	*REG32(GBL_A8_MAP_ADDR_REG) = gd->cpu2_run_addr;

	*REG32(GBL_CFG_ARM_CLK_REG) |= (0x1 << 1);		// CPU2 clock enable

	*REG32(GBL_ARM_RST_REG) &= ~(0x1 << 3);		// Hold CPU2
	*REG32(GBL_ARM_RST_REG) |= (0x1 << 2);		// Release CPU2 soft reset_n

	*REG32(0xFFF30004) |= (0x1 << 2);		// arm2_axiclk_sft_rstn

	*REG32(GBL_ARM_RST_REG) |= (0x1 << 3);

	return 0;
}
