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
 * @author:	changjun.li <lichangjun@innofidei.com>
 * @date:	2013-05-28
 *
 */
#define LOG_TAG		"nand_adap"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libmf.h>

#include <mf_debug.h>

#include <cyg/mtd/mtd.h>
#include <cyg/mtd/mtd_nand.h>

#define MAX_NAND_ID_LEN (8)

extern struct mtd_info mtd_nand_info[CFG_MAX_NAND_DEVICE];

/**
 * @brief get mtd device info
 *
 * @param[out] mtd:	MTD device structure
 * @param[out] chip: NAND chip descriptor
 *
 * @return : if success return 0, otherwise return -1.
 */
static int get_mtd_info(struct mtd_info **mtd, struct nand_chip **chip)
{
	*mtd = (struct mtd_info *) (&mtd_nand_info[0]);
	*chip = (struct nand_chip *) (*mtd)->priv;

	if (NULL != (*mtd)->name) {
		if (0 != strcmp((*mtd)->name, "nand")) {
			MFLOGE("get mtd info error mtd->name%s\n", (*mtd)->name);
			return -1;
		}

	} else {
		MFLOGE("mtd info null \n");
		return -1;
	}

	if (NULL == *chip) {
		MFLOGE("chip info null \n");
		return -1;
	}

	return 0;
}

/**
 * @brief  mailbox nand readID
 *
 * @param[in] buf:	id buf
 * @param[in] len:	buf len
 * @param[out] retlen:	actully got id len
 *
 * @return : if success return 0, otherwise return -1.
 */
int mf_NandReadID(unsigned char *buf, int len, int *retlen)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	int i;

	if (0 != get_mtd_info(&mtd, &this)) {
		return -1;
	}

	if (len > MAX_NAND_ID_LEN) {
		len = MAX_NAND_ID_LEN;
	}

	/* Send the command for reading device ID */
	this->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read ID string */
	for (i = 0; i < len; i++) {
		buf[i] = this->read_byte(mtd);
	}

	*retlen = len;

	return 0;
}

/**
 * @brief  mailbox nand get info
 *
 * @param[out] pagesize:	nand pagesize
 * @param[out] blockszie:	nand blocksize
 * @param[out] chipsize::	nand chipsize
 * @param[out] iswidth16:	whether nand bus with is 16
 *
 * @return : if success return 0, otherwise return -1.
 */
int mf_NandGetInfoCallback(unsigned long *pagesize, unsigned long *blockszie, unsigned long *chipsize, int *iswidth16)
{
	struct mtd_info *mtd;
	struct nand_chip *this;

	if (0 != get_mtd_info(&mtd, &this)) {
		return -1;
	}

	if (NULL != pagesize) {
		*pagesize = mtd->oobblock;
	}

	if (NULL != blockszie) {
		*blockszie = mtd->erasesize;
	}

	if (NULL != chipsize) {
		*chipsize = this->chipsize;
	}

	if (NULL != iswidth16) {
		*iswidth16 = this->options & NAND_BUSWIDTH_16;
	}

	return 0;
}

/**
 * @brief   control CE line
 *
 * @param[in] chip:	chipnumber to select, -1 for deselect
 *
 * @return : if success return 0, otherwise return -1.
 */
int mf_NandChipSelectCallback(int chip)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	
	if(0 != get_mtd_info(&mtd, &this)){
		return -1;
	}
	
	this->select_chip(mtd, chip);

	return 0;
}

/**
 * @brief   read  page data
 *
 * @param[in] page:	page number to read
 * @param[in] column:	   the column address for this command, -1 if none
 * @param[in] buf:	    buffer to store read data
 * @param[in] len:	    buffer len
 * @param[out] retlen:	actually read data len
 *
 * @return : if success return 0, otherwise return -1.
 */
int mf_NandReadPage(int page, int column, void *buf, int len, int *retlen)
{
	struct mtd_info *mtd;
	struct nand_chip *this;

	if (0 != get_mtd_info(&mtd, &this)) {
		return -1;
	}

	this->cmdfunc(mtd, NAND_CMD_READ0, column, page);
	this->read_buf(mtd, buf, len);

	*retlen = len;

	return 0;
}

/**
 * @brief write  page data
 *
 * @param[in] page:	page number to write
 * @param[in] column: the column address for this command, -1 if none
 * @param[in] buf:	    buffer to store write data
 * @param[in] len:	    buffer len
 * @param[out] retlen:	actually writen data len
 *
 * @return : if success return 0, otherwise return -1.
 */
int mf_NandWritePage(int page, int column, const void *buf, int len, int *retlen)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	int status = 0;

	if (0 != get_mtd_info(&mtd, &this)) {
		return -1;
	}

	this->cmdfunc(mtd, NAND_CMD_SEQIN, column, page);
	this->write_buf(mtd, buf, len);
	this->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	/* call wait ready function */
	status = this->waitfunc(mtd, this, FL_WRITING);
	/* See if device thinks it succeeded */
	if (status & 0x01) {
		MFLOGE("Failed write, page 0x%08x, ", page);
		return -1;
	}

	*retlen = len;

	return 0;
}

/**
 * @brief erase  page
 *
 * @param[in] page:	page number to erase
 *
 * @return : if success return 0, otherwise return -1.
 */
int mf_NandEraseBlock(int page)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	int status = 0;

	if (0 != get_mtd_info(&mtd, &this)) {
		return -1;
	}

	this->erase_cmd(mtd, page & this->pagemask);

	status = this->waitfunc(mtd, this, FL_ERASING);

	/* See if block erase succeeded */
	if (status & NAND_STATUS_FAIL) {
		MFLOGE("Failed erase, page 0x%08x\n", page);
		return -1;
	}

	return 0;
}

