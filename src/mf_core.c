/**
 *
 * MiFi initialization
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		mifi subsystem initialization
 *
 * @author:
 *		jimmy.li <lizhengming@innofidei.com>
 *
 * @date:	2013-04-09
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/kernel/kapi.h>

#include <mf_comn.h>
#include <libmf.h>

static CYGBLD_ATTRIB_ALIGN(4) unsigned char __noncached_buffer[0x100000] CYGBLD_ATTRIB_SECTION(".dma");
//TODO : must be specified manually
#define NONCACHED_BUFFER_START	((unsigned long)__noncached_buffer)
#define NONCACHED_BUFFER_LEN	(sizeof(__noncached_buffer))


MF_SYSINIT extern int mf_memp_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_netbuf_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_board_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_mreqb_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_micproto_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_mbserial_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_mbnand_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_netif_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_atcmd_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_log_init(mf_gd_t *gd);
MF_SYSINIT extern int mf_boot_cpu2(mf_gd_t *gd);


/* global configuration data */
static mf_gd_t	mf_gd = {
	.max_mbser_num	= 3,
	.at_mbser_line	= 0,
	.log_mbser_line	= 1,
	.cpu2_run_addr	= 0x41900000,		//FIXME : why such 0x46800000 will fail ?
	.mbox_reserve_addr = NONCACHED_BUFFER_START,
	.mbox_reserve_size = NONCACHED_BUFFER_LEN,
};

static mf_init_func_t mf_stage1_init_func_table[] = {
	mf_memp_init,
	mf_netbuf_init,
	mf_board_init,
	mf_mreqb_init,
	mf_micproto_init,
	mf_mbserial_init,
	mf_mbnand_init,
	mf_netif_init,
};

static mf_init_func_t mf_stage2_init_func_table[] = {
	mf_atcmd_init,
	mf_log_init,
	mf_boot_cpu2,
};



int mf_subsystem_init(void)
{
	int i;
	int ret;
	mf_init_func_t init;

	for (i=0; i < ARRAY_SIZE(mf_stage1_init_func_table); i++) {
		init = mf_stage1_init_func_table[i];
		ret = init(&mf_gd);
		if (ret)
		  return ret;
	}

	for (i=0; i < ARRAY_SIZE(mf_stage2_init_func_table); i++) {
		init = mf_stage2_init_func_table[i];
		ret = init(&mf_gd);
		if (ret)
		  return ret;
	}

	return 0;
}
