/**
 *
 * nand interface between Mifi and LTE platform.
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		nand interface for mifi
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2013-05-08
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libmf.h>

#define LOG_TAG		"nand_adap"
#include <mf_debug.h>

#define nandp_report_once()	\
({										\
	static int _report_once = 0;		\
										\
	if (!_report_once) {				\
		_report_once = 1;				\
		MFLOGE("function %s not implement!\n", __FUNCTION__);	\
	}									\
										\
})

__attribute__((weak)) int mf_NandReadIDCallback(unsigned char *buf, int len, int *retlen)
{
	nandp_report_once();
	return -1;
}

__attribute__((weak)) int mf_NandGetInfoCallback(unsigned long *pagesize, unsigned long *blockszie, unsigned long *chipsize, int *iswidth16)
{
	nandp_report_once();
	return -1;
}

__attribute__((weak)) int mf_NandChipSelectCallback(int chip)
{
	nandp_report_once();
	return -1;
}

__attribute__((weak)) int mf_NandReadPageCallback(int page, int column, void *buf, int len, int *retlen)
{
	nandp_report_once();
	return -1;
}

__attribute__((weak)) int mf_NandWritePageCallback(int page, int column, const void *buf, int len, int *retlen)
{
	nandp_report_once();
	return -1;
}

__attribute__((weak)) int mf_NandEraseBlockCallback(int page)
{
	nandp_report_once();
	return -1;
}

