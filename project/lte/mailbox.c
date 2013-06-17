/**
 *
 * mailbox interface between Mifi and LTE platform.
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
#define MFLOG_TAG		"mbox_adap"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/hal/hal_arch.h>           // CYGNUM_HAL_STACK_SIZE_TYPICAL
#include <cyg/kernel/kapi.h>
#include <mf_comn.h>
#include <mf_mailbox.h>
#include <libmf.h>

#include <mf_debug.h>

static cyg_handle_t	os_mbox_handle;
static cyg_mbox		os_mbox_obj;

/**
 * @brief send a mailbox message to CPU2
 * 
 * @param[in] msg : the message to be sent to CPU2.
 *
 * @return : if success return 0, otherwise return -1.
 *
 * @note : this function should implement by PS.
 */
MF_CALLBACK __attribute__((weak)) int mf_MboxMsgSendToCpu2(unsigned long msg)
{
	return 0;
}

/**
 * @brief notify a new mailbox message from CPU2.
 *
 * @param[in] msg : the received message from CPU2
 *
 * @return : void
 *
 * @note : this function call by PS when a new mailbox message received from CPU2.
 */
MF_API void mf_MboxMsgRecvFromCpu2(unsigned long msg)
{
	if (os_mbox_handle != (cyg_handle_t)NULL)
		cyg_mbox_tryput(os_mbox_handle, (void*)msg);
}


/**
 * @brief
 */
mf_mbox_t mf_mbox_get(const char* name)
{
	cyg_mbox_create(&os_mbox_handle, &os_mbox_obj);

	return (mf_mbox_t)1;
}

/**
 * @brief
 */
int mf_mbox_msg_put(mf_mbox_t _mbox, mbox_msg_t msg)
{
	msg = out_msg_fixup(msg);

	return mf_MboxMsgSendToCpu2(msg);
}

/**
 * @brief
 */
mbox_msg_t mf_mbox_msg_get(mf_mbox_t _mbox)
{
	mbox_msg_t msg;

	msg = (mbox_msg_t)cyg_mbox_get(os_mbox_handle);		// may block

	return in_msg_fixup(msg);
}
