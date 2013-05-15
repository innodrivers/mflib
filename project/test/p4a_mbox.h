
#ifndef _P4A_MBOX_H
#define _P4A_MBOX_H

enum P4A_MBOX_ID {
	P4A_MBOX_ID_0 = 0,      // CPU1
	P4A_MBOX_ID_1,          // DSP
	P4A_MBOX_ID_2,          // CPU 2
	P4A_MBOX_ID_3,
	P4A_MBOX_ID_4,
	P4A_MBOX_ID_MAX,
};

#define MAILBOX_INTSTATUS(m)    (0x00 + (m) * 0x40)
#define MAILBOX_RAWINTSTATUS(m) (0x08 + (m) * 0x40)
#define MAILBOX_INTMASK(m)      (0x10 + (m) * 0x40)
#define MAILBOX_MESSAGE(m)      (0x18 + (m) * 0x40)
#define MAILBOX_FIFOSTATUS(m)   (0x20 + (m) * 0x40)
#define MAILBOX_MSGSTATUS(m)    (0x28 + (m) * 0x40)
#define MAILBOX_THRESHOLD(m)    (0x30 + (m) * 0x40)
#define MAILBOX_SOFTINTTRIGGER  0x13c

/* bit define for INTSTATUS, RAWINTSTATUS and INTMASK */
#define MBOX_NOTFULL_BIT(m)     (0x1 << ((m) + 1))
#define MBOX_NEWMSG_BIT         (0x1)
#define MBOX_SWINT_BIT          (0x1 << 6)

/* bit define for FIFOSTATUS */
#define MBOX_FIFO_FULL_BIT      (0x1 << 0)
#define MBOX_FIFO_EMPTY_BIT     (0x1 << 1)


struct p4a_mbox;
typedef int p4a_mbox_irq_t;
#define IRQ_TX      ((p4a_mbox_irq_t) 1)
#define IRQ_RX      ((p4a_mbox_irq_t) 2)

struct p4a_mbox_fifo {
	unsigned long msg;
	unsigned long fifo_stat;
	unsigned long msg_stat;
};

struct p4a_mbox_priv {
	struct p4a_mbox_fifo tx_fifo;
	struct p4a_mbox_fifo rx_fifo;
	unsigned long threshold;
	unsigned long int_stat;
	unsigned long int_mask;
	unsigned long rawint_stat;

	u32 notfull_bit;
	u32 newmsg_bit;
};

struct p4a_mbox_ops {
	int (*startup)(struct p4a_mbox *);
	int (*shutdown)(struct p4a_mbox *);

	mbox_msg_t (*fifo_read)(struct p4a_mbox *);
	void (*fifo_write)(struct p4a_mbox *, mbox_msg_t);
	int (*fifo_empty)(struct p4a_mbox *);
	int (*fifo_full)(struct p4a_mbox *);

	void (*enable_irq)(struct p4a_mbox *, p4a_mbox_irq_t);
	void (*disable_irq)(struct p4a_mbox *, p4a_mbox_irq_t);
	void (*ack_irq)(struct p4a_mbox *, p4a_mbox_irq_t);
	int (*is_irq)(struct p4a_mbox *, p4a_mbox_irq_t);
};

struct p4a_mbox {
	char* name;
	struct p4a_mbox_ops *ops;
	void *priv;

	cyg_handle_t	intr_hdl;
	cyg_interrupt	intr_obj;

	/* IN FIFO */
	cyg_handle_t	os_inmbox_hdl;
	cyg_mbox		os_inmbox_obj;
	int				inmbox_full;

	/* OUT FIFO */
	cyg_handle_t	os_outmbox_hdl;
	cyg_mbox		os_outmbox_obj;
	struct mf_work	tx_work;
};

#endif	/* _P4A_MBOX_H */
