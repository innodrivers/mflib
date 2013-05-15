
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/kernel/kapi.h>

#include <mf_comn.h>
#include <mf_thread.h>
#include <mf_debug.h>
#include <hardware.h>
#include <iomux.h>

static  p4a_mux_pin_t p4abu_iomux_cfgs[] = {

};

MF_SYSINIT int mf_board_init(mf_gd_t *gd)
{
#ifdef PROJECT_TEST
	p4a_iomux_config(p4abu_iomux_cfgs, ARRAY_SIZE(p4abu_iomux_cfgs));

	/* Mailbox */
	*REG32(GBL_CFG_BUS_CLK_REG) |= (1<<6);		// mailbox_hclk_en
	*REG32(GBL_CFG_SOFTRST_REG) |= (1<<7);		// release mailbox_rstn

#endif

	return 0;
}
