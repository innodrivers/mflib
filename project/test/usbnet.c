
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <cyg/kernel/kapi.h>

#include <mf_comn.h>
#include <mf_cache.h>
#include <mf_debug.h>

#include <cyg/io/usb/usbs_composite_serial.h>

extern usbs_composite_serial_t usbs_comp_ser0;
extern int usbs_comp_serial_rx(usbs_composite_serial_t* ser, void* buf, int n);
extern void usbs_serials_start(int num);

int usbnet_queue_rx(void * buf, int len)
{
//	mf_cache_inv_range(buf, len);

	return usbs_comp_serial_rx(&usbs_comp_ser0, buf, len);
}

int usbnet_queue_tx(const void * buf, int len)
{
	mf_cache_clean_range((void*)buf, len);

	return usbs_comp_serial_tx(&usbs_comp_ser0, buf, len);
}

int usbnet_init(void)
{
	usbs_serials_start(1);

	usbs_composite_wait_until_configured();

	return 0;
}
