/**
 *
 * mailbox driver 
 *
 * Copyright 2013 Innofidei Inc.
 *
 * @file
 * @brief
 *		mailbox driver for mifi
 *
 * @author:	jimmy.li <lizhengming@innofidei.com>
 * @date:	2013-04-10
 *
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/hal/drv_api.h>
#include <cyg/kernel/kapi.h>
#include <mf_comn.h>
#include <mf_mailbox.h>
#include <mf_thread.h>
#include <libmf.h>
#include <hardware.h>
#include "p4a_mbox.h"

#define MFLOG_TAG	"p4a_mbox"
#include <mf_debug.h>

extern void set_irq_source(devIntrSource_E SourceNum, bool IRQorFIQ, unsigned char Priority);

static inline mbox_msg_t out_msg_fixup(mbox_msg_t msg)
{
	return msg;
}

static inline mbox_msg_t in_msg_fixup(mbox_msg_t msg)
{
	return msg;
}

/*--------------------------------------------------------------*/
static unsigned long p4a_mbox_base = P4A_MBOX_BASE;
static int mbox_irqnum = IRQ_MAILBOX_DSP2ARM;

static inline u32 mbox_read_reg(struct p4a_mbox *mbox, int off)
{
	return *REG32(p4a_mbox_base + off);
}

static inline void mbox_write_reg(struct p4a_mbox* mbox, int off, u32 val)
{
	*REG32(p4a_mbox_base + off) = val;
}

static int p4a_mbox_startup(struct p4a_mbox *mbox)
{
	struct p4a_mbox_priv* p = mbox->priv;

	mbox_write_reg(mbox, p->int_mask, ~0x0);
	mbox_write_reg(mbox, p->rawint_stat, ~0x0);     //W1C
	mbox_read_reg(mbox, p->int_stat);

	mbox_write_reg(mbox, p->threshold, 0x1);

	return 0;
}

static int p4a_mbox_shutdown(struct p4a_mbox *mbox)
{
	return 0;
}

static mbox_msg_t p4a_mbox_fifo_read(struct p4a_mbox * mbox)
{
	struct p4a_mbox_priv* p = mbox->priv;
	struct p4a_mbox_fifo *rx_fifo = &p->rx_fifo;

	return (mbox_msg_t)mbox_read_reg(mbox, rx_fifo->msg);
}

static void p4a_mbox_fifo_write(struct p4a_mbox *mbox, mbox_msg_t msg)
{
	struct p4a_mbox_priv* p = mbox->priv;
	struct p4a_mbox_fifo *tx_fifo = &p->tx_fifo;

	return mbox_write_reg(mbox, tx_fifo->msg, msg);
}

static int p4a_mbox_fifo_empty(struct p4a_mbox *mbox)
{
	struct p4a_mbox_priv* p = mbox->priv;
	struct p4a_mbox_fifo *rx_fifo = &p->rx_fifo;

	return ((mbox_read_reg(mbox, rx_fifo->fifo_stat) & MBOX_FIFO_EMPTY_BIT) != 0);
}

static int p4a_mbox_fifo_full(struct p4a_mbox *mbox)
{
	struct p4a_mbox_priv* p = mbox->priv;
	struct p4a_mbox_fifo *tx_fifo = &p->tx_fifo;

	return ((mbox_read_reg(mbox, tx_fifo->fifo_stat) & MBOX_FIFO_FULL_BIT) != 0);
}

static void p4a_mbox_enable_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	struct p4a_mbox_priv* p = mbox->priv;
	u32 val, bit = (irq == IRQ_TX) ? p->notfull_bit : p->newmsg_bit;

	val = mbox_read_reg(mbox, p->int_mask);
	val &= ~bit;
	mbox_write_reg(mbox, p->int_mask, val);
}

static void p4a_mbox_disable_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	struct p4a_mbox_priv* p = mbox->priv;
	u32 val, bit = (irq == IRQ_TX) ? p->notfull_bit : p->newmsg_bit;

	val = mbox_read_reg(mbox, p->int_mask);
	val |= bit;
	mbox_write_reg(mbox, p->int_mask, val);
}

static void p4a_mbox_ack_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	//TODO
}

static int p4a_mbox_is_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	struct p4a_mbox_priv* p = mbox->priv;
	u32 bit = (irq == IRQ_TX) ? p->notfull_bit : p->newmsg_bit;
	u32 status;

	status = mbox_read_reg(mbox, p->int_stat);

	return ((status & bit) != 0);
}


static inline mbox_msg_t mbox_fifo_read(struct p4a_mbox *mbox)
{
	return mbox->ops->fifo_read(mbox);
}

static inline void mbox_fifo_write(struct p4a_mbox *mbox, mbox_msg_t msg)
{
	mbox->ops->fifo_write(mbox, msg);
}

static inline int mbox_fifo_empty(struct p4a_mbox *mbox)
{
	return mbox->ops->fifo_empty(mbox);
}

static inline int mbox_fifo_full(struct p4a_mbox *mbox)
{
	return mbox->ops->fifo_full(mbox);
}

static inline void mbox_ack_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	if (mbox->ops->ack_irq)
	  mbox->ops->ack_irq(mbox, irq);
}

static inline void mbox_enable_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	if (mbox->ops->enable_irq)
	  mbox->ops->enable_irq(mbox, irq);
}

static inline void mbox_disable_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	if (mbox->ops->disable_irq)
	  mbox->ops->disable_irq(mbox, irq);
}

static inline int mbox_is_irq(struct p4a_mbox *mbox, p4a_mbox_irq_t irq)
{
	return mbox->ops->is_irq(mbox, irq);
}

static cyg_uint32 p4a_mbox_isr(cyg_vector_t vector, cyg_addrword_t data)
{
	cyg_drv_interrupt_mask(vector);
	cyg_drv_interrupt_acknowledge(vector);

	return CYG_ISR_HANDLED|CYG_ISR_CALL_DSR;
}

static void __mbox_rx_interrupt(struct p4a_mbox *mbox)
{
	static mbox_msg_t msg = 0;		//store last no push message

	if (msg != 0) {
		goto push_msg;
	}

	while (!mbox_fifo_empty(mbox)) {
		msg = in_msg_fixup(mbox_fifo_read(mbox));
push_msg:
		if (!cyg_mbox_tryput(mbox->os_inmbox_hdl, (void*)msg)) {
			// os mbox full
			MFLOGI("soft FIFO full when rx msg\n");

			mbox_disable_irq(mbox, IRQ_RX);
			mbox->inmbox_full = 1;
			goto nomem;
		}
	}
	msg = 0;

	mbox_ack_irq(mbox, IRQ_RX);
nomem:
	return;
}

static void __mbox_tx_interrupt(struct p4a_mbox* mbox)
{
	mbox_disable_irq(mbox, IRQ_TX);
	mbox_ack_irq(mbox, IRQ_TX);

	mf_schedule_work(&mbox->tx_work);
}

static void p4a_mbox_tx_work(struct mf_work *work)
{
	struct p4a_mbox *mbox = work->priv;
	unsigned long msg;

	while (cyg_mbox_peek(mbox->os_outmbox_hdl)) {

		if (mbox_fifo_full(mbox)) {
			mbox_enable_irq(mbox, IRQ_TX);
			break;
		}

		msg = (unsigned long)cyg_mbox_get(mbox->os_outmbox_hdl);

		mbox_fifo_write(mbox, msg);
	}
}

static struct p4a_mbox mbox_cpu2_info;

static void p4a_mbox_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
	struct p4a_mbox *mbox = (struct p4a_mbox*)data;

	if (mbox_is_irq(mbox, IRQ_RX)) {
		__mbox_rx_interrupt(mbox);
	}

	if (mbox_is_irq(mbox, IRQ_TX)) {
		 __mbox_tx_interrupt(mbox);
	}

    cyg_drv_interrupt_unmask(vector);
}

static int mf_mbox_startup(struct p4a_mbox *mbox)
{
	*REG32(GBL_CFG_PERI_CLK_REG) |= (0x1 << 6);		// mailbox hclk enable
	*REG32(GBL_CFG_SOFTRST_REG) |= (0x1 << 7);	// release Mailbox Reset_n

	if (mbox->ops->startup)
		mbox->ops->startup(mbox);

	cyg_mbox_create(&mbox->os_inmbox_hdl, &mbox->os_inmbox_obj);
	cyg_mbox_create(&mbox->os_outmbox_hdl, &mbox->os_outmbox_obj);

	MF_INIT_WORK(&mbox->tx_work, p4a_mbox_tx_work, mbox);

	cyg_drv_interrupt_create(
						(cyg_vector_t)mbox_irqnum,
						(cyg_priority_t)1,
						(cyg_addrword_t)mbox,
						(cyg_ISR_t *)p4a_mbox_isr,
						(cyg_DSR_t *)p4a_mbox_dsr,
						&mbox->intr_hdl,
						&mbox->intr_obj);
	cyg_drv_interrupt_attach(mbox->intr_hdl);
	cyg_drv_interrupt_unmask(mbox_irqnum);

	set_irq_source(mbox_irqnum, SEL_IRQ, mbox_irqnum);

	mbox_enable_irq(mbox, IRQ_RX);
	return 0;
}

/**
 * @brief send a message to mailbox
 *
 * @param[in] _mbox : mailbox target
 * @param[in] msg : message to be sent
 *
 * @return : if success return 0, otherwise return -1.
 */
int mf_mbox_msg_put(mf_mbox_t _mbox, mbox_msg_t msg)
{
	struct p4a_mbox *mbox = (struct p4a_mbox*)_mbox;

	msg = out_msg_fixup(msg);

	/* 
	 * no msg pending in kernel FIFO and mailbox FIFO no full,
	 * in this case, write msg into mailbox FIFO directly.
	 */
	if (cyg_mbox_peek(mbox->os_outmbox_hdl) == 0 && !mbox_fifo_full(mbox)) {
		mbox_fifo_write(mbox, msg);
		return 0;
	}

	if (!cyg_mbox_tryput(mbox->os_outmbox_hdl, (void*)msg)) {
		// os mbox full
		MFLOGI("soft FIFO full when tx msg\n");
		return -1;
	}

	mf_schedule_work(&mbox->tx_work);

	return 0;
}

/**
 * @brief get a message from mailbox
 *
 * @param[in] _mbox : the mailbox instance
 *
 * @return : a message from mailbox
 *
 * @note : if mailbox empty, then this function will be block util a new message available.
 */
mbox_msg_t mf_mbox_msg_get(mf_mbox_t _mbox)
{
	struct p4a_mbox *mbox = (struct p4a_mbox*)_mbox;
	mbox_msg_t msg;

	//may block here when mailbox empty
	msg = (mbox_msg_t)cyg_mbox_get(mbox->os_inmbox_hdl);

	if (mbox->inmbox_full) {
		mbox->inmbox_full = 0;
		mbox_enable_irq(mbox, IRQ_RX);
	}

	return msg;
}

static struct p4a_mbox_ops p4a_mbox_ops = {
	.startup    = p4a_mbox_startup,
	.shutdown   = p4a_mbox_shutdown,
	.fifo_read  = p4a_mbox_fifo_read,
	.fifo_write = p4a_mbox_fifo_write,
	.fifo_empty = p4a_mbox_fifo_empty,
	.fifo_full  = p4a_mbox_fifo_full,
	.enable_irq = p4a_mbox_enable_irq,
	.disable_irq = p4a_mbox_disable_irq,
	.ack_irq    = p4a_mbox_ack_irq,
	.is_irq     = p4a_mbox_is_irq,
};


static struct p4a_mbox_priv p4a_mbox_cpu2_priv = {
	.tx_fifo = {
		.msg    = MAILBOX_MESSAGE(P4A_MBOX_ID_2),
		.fifo_stat  = MAILBOX_FIFOSTATUS(P4A_MBOX_ID_2),
		.msg_stat   = MAILBOX_MSGSTATUS(P4A_MBOX_ID_2),
	},
	.rx_fifo = {
		.msg    = MAILBOX_MESSAGE(P4A_MBOX_ID_0),
		.fifo_stat  = MAILBOX_FIFOSTATUS(P4A_MBOX_ID_0),
		.msg_stat   = MAILBOX_MSGSTATUS(P4A_MBOX_ID_0),
	},
	.threshold = MAILBOX_THRESHOLD(P4A_MBOX_ID_0),
	.int_stat   = MAILBOX_INTSTATUS(P4A_MBOX_ID_0),
	.int_mask   = MAILBOX_INTMASK(P4A_MBOX_ID_0),
	.rawint_stat    = MAILBOX_RAWINTSTATUS(P4A_MBOX_ID_0),
	.notfull_bit    = MBOX_NOTFULL_BIT(P4A_MBOX_ID_2),
	.newmsg_bit     = MBOX_NEWMSG_BIT,
};

/* mailbox communicate with CPU2 */
static struct p4a_mbox mbox_cpu2_info = {
	.name	= "CPU2",
	.ops	= &p4a_mbox_ops,
	.priv	= &p4a_mbox_cpu2_priv,
};

static struct p4a_mbox *p4a_mboxes[] = { &mbox_cpu2_info, NULL };


/**
 * @brief get a instance of mailbox to operate
 *
 * @param[in] name : mailbox name
 *
 * @return : the instance of mailbox
 */
mf_mbox_t mf_mbox_get(const char* name)
{
	struct p4a_mbox *mbox;
	int ret;

	for (mbox = *p4a_mboxes; mbox; mbox++) {
		if (!strcmp(mbox->name, name))
			break;
	}

	if (mbox==NULL)
		return (mf_mbox_t)NULL;

	ret = mf_mbox_startup(mbox);
	if (ret) {
		return (mf_mbox_t)NULL;
	}

	return (mf_mbox_t)mbox;
}

