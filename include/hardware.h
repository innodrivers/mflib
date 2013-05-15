#ifndef _MF_HARDWARE_H
#define _MF_HARDWARE_H

#define REG8(addr)			((volatile unsigned char *)(addr))
#define REG16(addr)			((volatile unsigned short *)(addr))
#define REG32(addr)			((volatile unsigned long *)(addr))

#define P4A_MBOX_BASE		0xE0700000
#define P4A_PMU_BASE        0xE1300000
#define P4A_GBL_BASE		0xE1200000

/* PMU registers */
#define PMU_REG(off)        (P4A_PMU_BASE + (off))

#define PMU_CTRL_REG            PMU_REG(0x00)

#define PMU_PLL1_CTRL_REG       PMU_REG(0x74)
#define PMU_PLL2_CTRL_REG       PMU_REG(0x78)
#define PMU_PLL3_CTRL_REG       PMU_REG(0xa4)

#define PMU_CLKRST1_REG         PMU_REG(0x250)
#define PMU_CLKRST2_REG         PMU_REG(0x254)
#define PMU_CLKRST3_REG         PMU_REG(0x258)
#define PMU_CLKRST4_REG         PMU_REG(0x25c)
#define PMU_CLKRST5_REG         PMU_REG(0x260)


/* Global registers */
#define GBL_REG(off)		(P4A_GBL_BASE + off)

#define GBL_CFG_ARM_CLK_REG     GBL_REG(0x0c)
#define GBL_CFG_DSP_CLK_REG     GBL_REG(0x10)
#define GBL_CFG_BUS_CLK_REG     GBL_REG(0x14)
#define GBL_CFG_AB_CLK_REG      GBL_REG(0x18)
#define GBL_CFG_PERI_CLK_REG    GBL_REG(0x1c)
#define GBL_CFG_SOFTRST_REG     GBL_REG(0x20)
#define GBL_BOOT_SEL_REG        GBL_REG(0x24)
#define GBL_CFG_DSP_REG         GBL_REG(0x28)
#define GBL_GPR_0_REG           GBL_REG(0x2c)
#define GBL_GPR_1_REG           GBL_REG(0x30)
#define GBL_GPR_2_REG           GBL_REG(0x34)
#define GBL_GPR_3_REG           GBL_REG(0x38)
#define GBL_CTRL_DDR_REG        GBL_REG(0x3c)
#define GBL_CTRL_DBG_REG        GBL_REG(0x40)
#define GBL_ARM_RST_REG         GBL_REG(0x6c)
#define GBL_CTRL_TOP_REG        GBL_REG(0x70)
#define GBL_CFG_ARM2_CLK_REG    GBL_REG(0x74)
#define GBL_CFG_PERI2_CLK_REG   GBL_REG(0x78)
#define GBL_CFG_DSP2_CLK_REG    GBL_REG(0x7c)
#define GBL_SMID_OCR_REG        GBL_REG(0x100)
#define GBL_SMID_CTL_REG        GBL_REG(0x104)
#define GBL_A8_MAP_ADDR_REG     GBL_REG(0x108)
#define GBL_PERI_HTRANS_REG     GBL_REG(0x160)


#endif	/* _MF_HARDWARE_H */
