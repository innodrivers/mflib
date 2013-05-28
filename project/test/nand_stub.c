/**
 *
 * Nand Driver Stub for Mailbox based Nand (nand proxy) in MiFi Project
 *
 * Copyright 2012 Innofidei Inc.
 *
 * @file
 * @brief
 *		nand  driver stub
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2013-01-16
 *
 */

#define MFLOG_TAG "mfnand"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/kernel/kapi.h>
#include <linux/list.h>
#include <mf_comn.h>
#include <mf_thread.h>

#include <mf_mbnand.h>
#include <mf_debug.h>

#ifndef KiB
#define KiB(x)			((x)<<10)
#endif

#ifndef MiB
#define MiB(x)			((x)<<20)
#endif

#ifndef Gib2MiB
#define Gib2MiB(x)		((x)<<7)
#endif

/*-------------changeable begin------ */
#define __NEM_IO_WIDTH	8 	//16

#define __NEM_PAGE_DATA_SIZE		(2048)
#define __NEM_PAGE_SPARE_SIZE		(64)
#define __NEM_BLOCK_SIZE			KiB(128)
#define __NEM_CHIP_SIZE				Gib2MiB(4)		// unit : MiB
/*-------------changeable end------ */

#if (__NEM_IO_WIDTH == 16)
#define NEM_PAGE_DATA_SIZE		((__NEM_PAGE_DATA_SIZE)>>1)
#define NEM_PAGE_SPARE_SIZE		((__NEM_PAGE_SPARE_SIZE)>>1)
#define NEM_BLOCK_SIZE			((__NEM_BLOCK_SIZE)>>1)
#define NEM_CHIP_SIZE			((__NEM_CHIP_SIZE)>>1)
#else
#define NEM_PAGE_DATA_SIZE		__NEM_PAGE_DATA_SIZE
#define NEM_PAGE_SPARE_SIZE		__NEM_PAGE_SPARE_SIZE
#define NEM_BLOCK_SIZE			__NEM_BLOCK_SIZE
#define NEM_CHIP_SIZE			__NEM_CHIP_SIZE
#endif

#define NEM_PAGE_TOTAL_SIZE		(NEM_PAGE_DATA_SIZE + NEM_PAGE_SPARE_SIZE)
#define NEM_PAGES_PER_BLOCK		(NEM_BLOCK_SIZE / NEM_PAGE_DATA_SIZE)
#define NEM_BLOCKS_PER_CHIP		((NEM_CHIP_SIZE << 10) / (NEM_BLOCK_SIZE >> 10))

static unsigned char nstub_id[] = {0x2c, 0xcc, 0x90, 0xd5, 0x56};	/* NAND ID */

typedef struct {
	unsigned char data[NEM_PAGE_TOTAL_SIZE];
}nandemul_page_t;

typedef struct {
	nandemul_page_t	*page[NEM_PAGES_PER_BLOCK];
	int is_bad;
}nandemul_block_t;

typedef struct {
	nandemul_block_t *block[NEM_BLOCKS_PER_CHIP];
}nandemul_chip_t;

static nandemul_chip_t	nstub;	/* represents a nand chip */

static inline int page_to_block(int page)
{
	return page / NEM_PAGES_PER_BLOCK;
}

static inline int page_to_pageinblock(int page)
{
	return page % NEM_PAGES_PER_BLOCK;
}

/**
 * @brief do nand id read
 * 
 * @param[out] buf - id buffer
 * @param[in] len - id buffer length
 * @param[out]retlen - actuall id length
 *
 * @return - if success return 0, otherwise return -1
 */
int mf_NandReadID(unsigned char* buf, int len, int *retlen)
{
	int idlen = sizeof(nstub_id) / sizeof(nstub_id[0]);
	int i;

	idlen = MIN(idlen, len);

	for (i=0; i<idlen; i++)
		buf[i] = nstub_id[i];

	if (retlen)
		*retlen = idlen;

	return 0;
}

/**
 * @brief do nand info get 
 * 
 * @param[out] pagesize - the page size in bytes
 * @param[out] blocksize - the erase block size in bytes
 * @param[out] chipsize - total chip size in Mega bytes
 * @param[out] iswidth16 - indicates nand IO width, 0 : width x8, 1 : width x16.
 *
 * @return - if success return 0, otherwise return -1
 */
int mf_NandGetInfo(unsigned long *pagesize,
					unsigned long *blocksize,
					unsigned long *chipsize,
					int *iswidth16)
{
	if (pagesize)
		*pagesize = NEM_PAGE_DATA_SIZE;
	if (blocksize)
		*blocksize = NEM_BLOCK_SIZE;
	if (chipsize)
		*chipsize = NEM_CHIP_SIZE;
	if (iswidth16)
		*iswidth16 = 0;

	return 0;
}

/**
 * @brief do nand chip select control 
 * 
 * @param[in] chip - chip select id, >=0 enable specified chip select, -1 disable chip select
 *
 * @return - if success return 0, otherwise return -1
 */
int mf_NandChipSelect(int chip)
{
	return 0;
}

/**
 * @brief do nand page read 
 * 
 * @param[in] page - page address to read
 * @param[in] column - column address in page
 * @param[in] buf - the buffer to store read data
 * @param[in] len - buffer length
 * @param[out] retlen - actual read length
 *
 * @return - if success return 0, otherwise return -1
 */
int mf_NandReadPage(int page, int column, void *buf, int len, int *retlen)
{
	int block, page_in_block;
	nandemul_block_t *nstub_block;
	nandemul_page_t *nstub_page;

	len = MIN(len, NEM_PAGE_TOTAL_SIZE - column);

	block = page_to_block(page);
	page_in_block = page_to_pageinblock(page); 

	if ((nstub_block = nstub.block[block]) == NULL || 
		(nstub_page = nstub_block->page[page_in_block]) == NULL) {
		memset(buf, 0xFF, len);

	} else {
		memcpy(buf, &nstub_page->data[column], len);
	}

	if (retlen)
		*retlen = len;

	return 0;
}

/**
 * @brief do nand page program 
 * 
 * @param[in] page - page address to be program
 * @param[in] column - column address in page
 * @param[in] buf - the content buffer to be program
 * @param[in] len - buffer length
 * @param[out] retlen - actual program length
 *
 * @return - if success return 0, otherwise return -1
 */
int mf_NandWritePage(int page, int column, const void *buf, int len, int *retlen)
{
	int block, page_in_block;
	nandemul_block_t * nstub_block;
	nandemul_page_t * nstub_page;

	len = MIN(len, NEM_PAGE_TOTAL_SIZE - column);


	block = page_to_block(page);
	page_in_block = page_to_pageinblock(page); 

	if (block >= NEM_BLOCKS_PER_CHIP) {
		MFLOGE("mf_NandWritePage: page address invalid!\n");
		return -1;
	}

	nstub_block = nstub.block[block];
	if (nstub_block == NULL) {
		nstub_block = malloc(sizeof(*nstub_block));
		memset((void*)nstub_block, 0, sizeof(*nstub_block));
		nstub.block[block] = nstub_block;
	}

	nstub_page = nstub_block->page[page_in_block];
	if (nstub_page == NULL) {
		nstub_page = malloc(sizeof(*nstub_page));
		memset((void*)nstub_page->data, 0xFF, sizeof(nstub_page->data));
		nstub_block->page[page_in_block] = nstub_page;
	}

	memcpy((void*)&nstub_page->data[column], buf, len);

	if (retlen)
		*retlen = len;

	return 0;
}

/**
 * @brief do nand block erase
 * 
 * @param[in] page - page address of block to erase
 *
 * @return - if success return 0, otherwise return -1
 */
int mf_NandEraseBlock(int page)
{
	int block;
	int i;
	nandemul_block_t * nstub_block;
	nandemul_page_t * nstub_page;

	block = page_to_block(page);

	nstub_block = nstub.block[block];
	if (nstub_block == NULL)
		return 0;

	for (i=0; i<NEM_PAGES_PER_BLOCK; i++) {
		nstub_page = nstub_block->page[i];
		if (nstub_page) {
			memset((void*)nstub_page->data, 0xFF, sizeof(nstub_page->data));
		}
	}

	return 0;
}

