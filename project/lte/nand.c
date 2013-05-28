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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libmf.h>

#define LOG_TAG		"nand_adap"
#include <mf_debug.h>

#include <cyg/mtd/mtd.h>
#include <cyg/mtd/mtd_nand.h>

#define MAX_NAND_ID_LEN (8)

extern struct mtd_info mtd_nand_info[CFG_MAX_NAND_DEVICE];

/**
 * get_mtd_info - get mtd device info
 * @mtd:	MTD device structure
 * @chip:	NAND chip descriptor
 *
 * get mtd device info
 */
static int get_mtd_info(struct mtd_info **mtd, struct nand_chip **chip)
{
	*mtd = (struct mtd_info *)(&mtd_nand_info[0]);
	*chip = (struct nand_chip *)(*mtd)->priv;
	if (NULL != (*mtd)->name){
		if(0 != strcmp((*mtd)->name, "nand")){
			MFLOGE("%s %d %s get mtd info error mtd->name%s\n",__FILE__,__LINE__, __FUNCTION__, (*mtd)->name);
			return -1;
		}
	}else{
		MFLOGE("%s %d %s mtd info null \n", __FILE__, __LINE__, __FUNCTION__);
		return -1;
	}
	
	if(NULL == *chip){
		MFLOGE("%s %d %s chip info null \n", __FILE__, __LINE__, __FUNCTION__);
		return -1;
	}
	return 0;
}
/**
 * mf_NandReadIDCallback - mailbox nand readID
 * @buf:	   id buf
 * @len:	   buf len
 * @retlen:	actully got id len
 */
__attribute__((weak)) int mf_NandReadIDCallback(unsigned char *buf, int len, int *retlen)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	int i;

	if(0 != get_mtd_info(&mtd, &this)){
		return -1;
	}

	if(len > MAX_NAND_ID_LEN){
		MFLOGE("%s %d req id len%d exceed maxlen \n", __FILE__, __LINE__,len);
		len = MAX_NAND_ID_LEN;
	}
	/* Send the command for reading device ID */
	this->cmdfunc (mtd, NAND_CMD_READID, 0x00, -1);

	/* Read ID string */
	for (i = 0; i < len; i++){
		buf[i] = this->read_byte(mtd);
	}
	*retlen = len;
	
	return 0;
}
/**
 * mf_NandReadIDCallback - mailbox nand get info
 * @pagesize:	nand pagesize 
 * @blockszie:	nand blocksize 
 * @chipsize::	nand chipsize 
 * @iswidth16:	whether nand bus with is 16
 *
 * mailbox nand get info
 */
__attribute__((weak)) int mf_NandGetInfoCallback(unsigned long *pagesize, unsigned long *blockszie, unsigned long *chipsize, int *iswidth16)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	
	if(0 != get_mtd_info(&mtd, &this)){
		return -1;
	}
	
	if(NULL != pagesize){
		*pagesize = mtd->oobblock;
	}
	
	if(NULL != blockszie){
		*blockszie = mtd->erasesize;
	}
	
	if(NULL != chipsize){
		*chipsize = this->chipsize;
	}
	
	if(NULL != iswidth16){
		*iswidth16 = this->options & NAND_BUSWIDTH_16;
	}

	return 0;
}
/**
 * mf_NandChipSelectCallback - control CE line
 * @chip:	chipnumber to select, -1 for deselect
 *
 * select function for 1 chip devices
 */
__attribute__((weak)) int mf_NandChipSelectCallback(int chip)
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
 * mf_NandReadPageCallback -read  page data
 * @buf:	    buffer to store read data
 * @buf:	    buffer len
 * @page:	page number to read
 * @column:	   the column address for this command, -1 if none
 * @retlen:	actually read data len
 */
__attribute__((weak)) int mf_NandReadPageCallback(int page, int column, void *buf, int len, int *retlen)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	
	if(0 != get_mtd_info(&mtd, &this)){
		return -1;
	}
	this->cmdfunc(mtd, NAND_CMD_READ0, column, page);
	this->read_buf(mtd, buf, len);

	*retlen = len;

	return 0;
}
/**
 * mf_NandWritePageCallback -write  page data
 * @buf:	    buffer to store write data
 * @buf:	    buffer len
 * @page:	page number to write
 * @column:	   the column address for this command, -1 if none
 * @retlen:	actually writen data len
 */
__attribute__((weak)) int mf_NandWritePageCallback(int page, int column, const void *buf, int len, int *retlen)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	int status = 0;
	
	if(0 != get_mtd_info(&mtd, &this)){
		return -1;
	}

	this->cmdfunc(mtd, NAND_CMD_SEQIN, column, page);
	this->write_buf(mtd, buf, len);
	this->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	/* call wait ready function */
	status = this->waitfunc (mtd, this, FL_WRITING);
	/* See if device thinks it succeeded */
	if (status & 0x01) {
		MFLOGE("%s: " "Failed write, page 0x%08x, ", __FUNCTION__, page);
		return -EIO;
	}
	*retlen = len;
	return 0;
}
/**
 * mf_NandEraseBlockCallback -erase  page 
 * @page:	page number to erase
  */
__attribute__((weak)) int mf_NandEraseBlockCallback(int page)
{
	struct mtd_info *mtd;
	struct nand_chip *this;
	int status = 0;
	
	if(0 != get_mtd_info(&mtd, &this)){
		return -1;
	}
	this->erase_cmd(mtd, page & this->pagemask);

	status = this->waitfunc(mtd, this, FL_ERASING);
	/* See if block erase succeeded */
	if (status & NAND_STATUS_FAIL) {
		MFLOGE("%s: Failed erase, "
			"page 0x%08x\n", __func__, page);
		return -EIO;
	}
	
	return 0;
}

