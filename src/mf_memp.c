/*
 * mf_memp.c
 *
 * Memory Pool Manager
 *
 * sources derives from lwIP project.
 *
 *  Created on: Apr 25, 2013
 *      Author: drivers
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mf_comn.h>
#include <libmf.h>

#include <mf_memp.h>
#include "mem.h"

struct mf_memp {
	struct mf_memp *next;
};

#define MEM_ALIGNMENT				4
#define MF_MEM_ALIGN_SIZE(size)		_ALIGN_UP((size), MEM_ALIGNMENT)
#define MF_MEM_ALIGN(addr)			_ALIGN_UP((int)(addr), MEM_ALIGNMENT)

#define MEMP_SIZE					MF_MEM_ALIGN_SIZE(sizeof(struct mf_memp))



/* This array holds the first free element of each pool,
 * Elements form a linked list */
static struct mf_memp *memp_tab[MF_MEMP_MAX];

/* This array holds the element sizes of each pool */
static const int memp_sizes[MF_MEMP_MAX] = {
#define MF_MEMPOOL(name, num, size, desc)	MF_MEM_ALIGN_SIZE(size),
#include <memp_std.h>
};

static const int memp_num[MF_MEMP_MAX] = {
#define MF_MEMPOOL(name, num, size, desc)	(num),
#include <memp_std.h>
};

static u8 memp_memory[ MEM_ALIGNMENT - 1
#define MF_MEMPOOL(name, num, size, desc)	+ ((num) * ((MEMP_SIZE) + MF_MEM_ALIGN_SIZE(size)))
#include <memp_std.h>
];

void* mf_memp_alloc(mf_memp_t type)
{
	struct mf_memp* memp;

	memp = memp_tab[type];

	if (memp != NULL) {
		memp_tab[type] = memp->next;

		memp = (struct mf_memp*)((u8*)memp + MEMP_SIZE);
	}

	return (struct mreqb*)memp;
}

void mf_memp_free(mf_memp_t type, void *mem)
{
	struct mf_memp *memp;

	if (mem == NULL)
		return;

	memp = (struct mf_memp*)((u8*)mem - MEMP_SIZE);

	memp->next = memp_tab[type];
	memp_tab[type] = memp;

}

MF_SYSINIT int mf_memp_init(mf_gd_t *gd)
{
	struct mf_memp* memp;
	int i, j;

	memp = (struct mf_memp*) MF_MEM_ALIGN(memp_memory);

	/* for every poll */
	for (i=0; i<MF_MEMP_MAX; i++) {
		memp_tab[i] = NULL;
		/* create a linked list of memp elements */
		for (j=0; j<memp_num[i]; j++) {
			memp->next = memp_tab[i];
			memp_tab[i] = memp;
			memp = (struct mf_memp *)((u8*)memp + MEMP_SIZE + memp_sizes[i]);
		}
	}

	return 0;
}
