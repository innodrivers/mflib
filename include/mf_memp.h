/*
 * mf_memp.h
 *
 *  Created on: Apr 25, 2013
 *      Author: drivers
 */

#ifndef MF_MEMP_H_
#define MF_MEMP_H_


typedef enum {
#define MF_MEMPOOL(name, num, size, desc)	MF_MEMP_##name,
#include <memp_std.h>
	MF_MEMP_MAX
}mf_memp_t;


extern void* mf_memp_alloc(mf_memp_t type);
extern void mf_memp_free(mf_memp_t type, void *mem);

#endif /* MF_MEMP_H_ */
