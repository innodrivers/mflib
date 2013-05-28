#ifndef _LIB_MF_H
#define _LIB_MF_H

#ifdef __cplusplus
extern "C" {
#endif

#define MF_API
#define MF_CALLBACK

/* AT */
typedef void (*mf_atcmd_rx_func_t)(void *data, int len);

MF_API int mf_ATSend(const char* data, int len);

MF_API int mf_ATCmdRxCallbackRegister(mf_atcmd_rx_func_t cb);

MF_CALLBACK void mf_ATCmdRx(void *data, int len);

/* LOG */
MF_API int mf_LogSend(const char* msg, int len);

MF_API void mf_LogPrintf(const char *fmt, ...);

MF_API void mf_LogvPrintf(const char *fmt, va_list ap);


/* NAND */
MF_CALLBACK int mf_NandReadID(unsigned char* buf, int len, int *retlen);

MF_CALLBACK int mf_NandGetInfo(unsigned long *pagesize, unsigned long *blockszie, unsigned long *chipsize, int *iswidth16);

MF_CALLBACK int mf_NandChipSelect(int chip);

MF_CALLBACK int mf_NandReadPage(int page, int column, void *buf, int len, int *retlen);

MF_CALLBACK int mf_NandWritePage(int page, int column, const void *buf, int len, int *retlen);

MF_CALLBACK int mf_NandEraseBlock(int page);

/* NETIF */
typedef int (*mf_netif_rx_func_t)(void* data, int len);

MF_API int mf_NetifLinkupInd(void);

MF_API int mf_NetifLinkdownInd(void);

MF_API int mf_NetifPacketXmit(void *ip_packet, int packet_len);

MF_API int mf_NetifPacketRxCallbackRegister(mf_netif_rx_func_t cb);

MF_CALLBACK int mf_NetifPacketRx(void *ip_packet, int packet_len);

/* Mailbox */
MF_CALLBACK int mf_MboxMsgSendToCpu2(unsigned long msg);

MF_API void mf_MboxMsgRecvFromCpu2(unsigned long msg);

#ifdef __cplusplus
}
#endif

#endif	/* _LIB_MF_H */
