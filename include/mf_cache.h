#ifndef _MF_CACHE_H_
#define _MF_CACHE_H_

#include <mf_comn.h>

#define DCACHE_BYTES_PER_LINE		32

#define CACHE_ADDR_ALIGNDOWN(addr)		_ALIGN_DOWN(addr, DCACHE_BYTES_PER_LINE)
#define CACHE_ADDR_ALIGNUP(addr)		_ALIGN_UP(addr, DCACHE_BYTES_PER_LINE)

extern void v7_dma_inv_range(unsigned int start, unsigned int end);
extern void v7_dma_flush_range(unsigned int start, unsigned int end);
extern void v7_dma_clean_range(unsigned int start, unsigned int end);

static inline void mf_cache_inv_range(void* start, int length)
{
	unsigned int _start;
	unsigned int _end;

	_start = CACHE_ADDR_ALIGNDOWN((unsigned int)start);
	_end = CACHE_ADDR_ALIGNUP(((unsigned int)(start) + length));

	v7_dma_inv_range(_start, _end);
}

static inline void mf_cache_flush_range(void *start, int length)
{
	unsigned int _start;
	unsigned int _end;

	_start = CACHE_ADDR_ALIGNDOWN((unsigned int)start);
	_end = CACHE_ADDR_ALIGNUP(((unsigned int)(start) + length));

	v7_dma_flush_range(_start, _end);
}

static inline void mf_cache_clean_range(void *start, int length)
{
	unsigned int _start;
	unsigned int _end;

	_start = CACHE_ADDR_ALIGNDOWN((unsigned int)start);
	_end = CACHE_ADDR_ALIGNUP(((unsigned int)(start) + length));

	v7_dma_clean_range(_start, _end);
}

#endif	/* _MF_CACHE_H_ */
