#define MFLOG_TAG	"usbnet"

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

#define USB_RXBUF_LEN		(2048)
#define USB_TXBUF_LEN		(2048)

#define USB_BUF_LEN			(USB_RXBUF_LEN + USB_TXBUF_LEN)

static CYGBLD_ATTRIB_ALIGN(4) unsigned char usb_buffer[USB_BUF_LEN] CYGBLD_ATTRIB_SECTION(".dma");

int usbnet_queue_rx(void *buf, int len)
{
	int ret;
	void *pbuf;

	pbuf = (void*)&usb_buffer[0];

	if (buf == NULL || len <=0 || len > USB_RXBUF_LEN) {
		MFLOGE("usbnet_queue_rx parameter invalid\n");
		return 0;
	}

	ret = usbs_comp_serial_rx(&usbs_comp_ser0, pbuf, len);
	if (ret > 0) {
		memcpy(buf, pbuf, ret);
	}

	return ret;
}

int usbnet_queue_tx(const void *buf, int len)
{
	void *pbuf;


	pbuf = (void*)&usb_buffer[USB_RXBUF_LEN];

	if (buf == NULL || len <= 0 || len > USB_TXBUF_LEN) {
		MFLOGE("usbnet_queue_tx parameter invalid\n");
		return 0;
	}

	memcpy(pbuf, buf, len);

	return usbs_comp_serial_tx(&usbs_comp_ser0, pbuf, len);
}

int usbnet_init(void)
{
	usbs_serials_start(1);

	usbs_composite_wait_until_configured();

	return 0;
}
