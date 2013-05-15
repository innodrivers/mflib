#ifndef _LIB_MF_H
#define _LIB_MF_H

#ifdef __cplusplus
extern "C" {
#endif

#define MF_API
#define MF_CALLBACK

/* AT Command Listen & Response Send */
typedef void (*mf_atcmd_listen_func_t)(void *data, int len);

MF_API int mf_ATSend(const char* data, int len);

MF_API int mf_ATCmdListenerRegister(mf_atcmd_listen_func_t cb);

MF_CALLBACK void mf_ATCmdRecvCallback(void *data, int len);

/* LOG Ouput */
MF_API void mf_LogPrintf(const char *fmt, ...);
MF_API void mf_LogvPrintf(const char *fmt, va_list ap);
MF_API int mf_LogSend(const char* msg, int len);

/* NAND Request */
MF_CALLBACK int mf_NandReadIDCallback(unsigned char* buf, int len, int *retlen);

MF_CALLBACK int mf_NandGetInfoCallback(unsigned long *pagesize, unsigned long *blockszie, unsigned long *chipsize, int *iswidth16);

MF_CALLBACK int mf_NandChipSelectCallback(int chip);

MF_CALLBACK int mf_NandReadPageCallback(int page, int column, void *buf, int len, int *retlen);

MF_CALLBACK int mf_NandWritePageCallback(int page, int column, const void *buf, int len, int *retlen);

MF_CALLBACK int mf_NandEraseBlockCallback(int page);

/* NETIF */
MF_CALLBACK int mf_NetifPacketRxCallback(void *ip_packet, int packet_len);

MF_API int mf_NetifLinkupInd(void);

MF_API int mf_NetifLinkdownInd(void);

MF_API int mf_NetifPacketXmit(void *ip_packet, int packet_len);

/* Mbox */
MF_CALLBACK int mf_MboxMsgSendToCpu2Callback(unsigned long msg);
MF_API void mf_MboxMsgRecvFromCpu2(unsigned long msg);

#ifdef __cplusplus
}
#endif

#endif	/* _LIB_MF_H */
