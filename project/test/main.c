/**
 * The Main Entrance of MiFi Project
 *
 * Copyright 2012 Innofidei Inc.
 *
 * @file
 * @brief
 *		the main entrance
 *
 * @author : drivers@innofidei.com
 *
 */
#define MFLOG_TAG		"main"

#include <stdio.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/infra/diag.h>
#include <pkgconf/kernel.h>

#include <hardware.h>
#include <mf_debug.h>


extern int netif_usbwan_init(void);

int main( int argc, char *argv[] )
{
	int i = 0;

	MFLOGI("Hello P4A!\n");
	while (1) {
		cyg_thread_delay(3000);
		MFLOGI("CPU1 alive %d\n", i++);

	}
}

extern int mf_subsystem_init(void);

void cyg_user_start(void)
{
	mf_subsystem_init();

	netif_usbwan_init();
}
