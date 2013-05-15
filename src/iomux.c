
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <hardware.h>

#include <mf_comn.h>
#include <libmf.h>
#include <iomux.h>


#define IOMUX_REG(i)	(P4A_PMU_BASE + 0x200 + (((i) - 1)<<2))		/* i : [1:18] */

typedef struct {
	union {
		unsigned long mpv;
		struct {
			u8	mode;
			u8	shift;
			u8	regid;
			u8	noused;
		};
	};
} mux_pin_s;

static inline u32 rd_iomux_regl(int iomux_regid)
{
	return *REG32(IOMUX_REG(iomux_regid));
}

static inline void wr_iomux_regl(int iomux_regid, u32 val)
{
	*REG32(IOMUX_REG(iomux_regid)) = val;
}

void p4a_iomux_config(p4a_mux_pin_t pins[], int num)
{
	int cached_regid = -1;
	unsigned long val = 0;
	int i, cnt = 0;

	DECL_MF_SYS_PROTECT(oldints);

	MF_SYS_PROTECT(oldints);
	
	for (i=0; i<num; i++) {
		mux_pin_s *p = (mux_pin_s*)(&pins[i]);

		if (p->regid < 1 || p->regid > 18)
			continue;
	
		if (cached_regid != p->regid) {
			if (cnt)
				wr_iomux_regl(cached_regid, val);

			cached_regid = p->regid;
			val = rd_iomux_regl(cached_regid);
			cnt = 0;
		}

		val &= ~(0xf << p->shift);
		val |= (p->mode << p->shift);
		cnt += 1;
	}

	if (cnt)
		wr_iomux_regl(cached_regid, val);
	MF_SYS_UNPROTECT(oldints);
}

