#ifndef _USB_NET_H_
#define _USB_NET_H_

extern int usbnet_queue_rx(void * buf, int len);
extern int usbnet_queue_tx(const void * buf, int len);
extern int usbnet_init(void);

#endif	/* _USB_NET_H_ */
