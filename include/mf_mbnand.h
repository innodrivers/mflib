/*
 * include/asm-arm/arch-p4a/p4a_mbnand.h
 *
 *  Copyright (C) 2012 Innofidei Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __ASM_ARCH_P4A_MBNAND_H
#define __ASM_ARCH_P4A_MBNAND_H

#define MAX_NAND_ID_LEN		(8)

enum {
	MB_NAND_READID = 0,
	MB_NAND_GETINFO,

	MB_NAND_CHIPSELECT,
	MB_NAND_READPAGE,
	MB_NAND_WRITEPAGE,
	MB_NAND_ERASEBLOCK,

	MB_NAND_READ,
	MB_NAND_WRITE,
	MB_NAND_ERASE,

	MB_NAND_CMD_MAX
};

enum {
	MB_NAND_RET_OK = 0,			/**< nand command execute successful */
	MB_NAND_RET_FAIL,			/**< nand command execute failed */
	MB_NAND_RET_NOTSUPPORT,		/**< nand command not support */
	MB_NAND_RET_ARG_INVALID,	/**< nand command argument invalid */
};

/* nand status bits */
#define MB_NAND_STATUS_FAIL		0x01	/* fail bit, OK : "0", fail : "1" */
#define MB_NAND_STATUS_READY	0x40	/* ready bit,  busy : "0", ready "1" */
#define MB_NAND_STATUS_WP		0x80	/* write protect bit,  protected: "0", not protected : "1" */


/* MB_NAND_READID argument */
struct mbnand_readid_arg {
	unsigned char id[MAX_NAND_ID_LEN];	/**< nand id */
	int len;		/**< actual nand id bytes */
};

/* MB_NAND_GETINFO argument */
struct mbnand_getinfo_arg {
	unsigned char id[MAX_NAND_ID_LEN];	/**< nand id */
	int id_len;					/**< actual nand id bytes */

	unsigned long pagesize;		/**< page size in bytes */
	unsigned long blocksize;	/**< the size of an erase block */
	unsigned long chipsize;		/**< total chip size in Mega bytes */
	int			  iowidth16;	/**< indicates io width, 0: width x8, 1: width x16 */

	unsigned long usable_block_start;	/**< the start block address of free usable space */
	unsigned long usable_block_end;		/**< the end block address of free usable space */
};

/* MB_NAND_CHIPSELECT argument */
struct mbnand_chipselect_arg {
	unsigned long cs;		/**< chip select id, >=0 : enable chip select, -1 : disable chip select */
};

/* MB_NAND_READPAGE argument */
struct mbnand_readpage_arg {
	unsigned long page;
	unsigned long column;
	unsigned char* buf;
	unsigned long len;

	unsigned long retlen;
};

/* MB_NAND_WRITEPAGE argument */
struct mbnand_writepage_arg {
	unsigned long page;		/**< page address to write [in]*/
	unsigned long column;	/**< column address in page to write [in] */
	unsigned char *buf;		/**< write buffer [out] */
	unsigned long len;		/**< buffer length [in] */

	unsigned long retlen;	/**< actual write size [out] */
	int status;				/**< nand status after program [out] */
};

/* MB_NAND_ERASEBLOCK argument */
struct mbnand_eraseblock_arg {
	unsigned long page;
	int status;
};


/* MB_NAND_READ argument */
struct mbnand_read_arg {
	unsigned long long offset;
	unsigned long len;
	unsigned char* buf;
	unsigned long readoob;

	unsigned long retlen;
};

/* MB_NAND_WRITE argument */
struct mbnand_write_arg {
	unsigned long long offset;
	unsigned long len;
	unsigned char* buf;
	unsigned long writeoob;

	unsigned long retlen;
};

/* MB_NAND_ERASE argument */
struct mbnand_erase_arg {
	unsigned long long offset;
	unsigned long long len;
};

typedef struct mbnand_arg {
	union {
		struct mbnand_readid_arg		readid;
		struct mbnand_getinfo_arg		getinfo;
		struct mbnand_chipselect_arg	chipselect;
		struct mbnand_readpage_arg		readpage;
		struct mbnand_writepage_arg		writepage;
		struct mbnand_eraseblock_arg	eraseblock;
		struct mbnand_read_arg			read;
		struct mbnand_write_arg			write;
		struct mbnand_erase_arg			erase;
	}u;
}mbnand_arg_t;

#endif
