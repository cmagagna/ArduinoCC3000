typedef void (*gcSpiHandleRx)(void *p);

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

extern void SpiOpen(gcSpiHandleRx pfRxHandler);

extern void SpiClose(void);

extern long SpiWrite(unsigned char *pUserBuffer, unsigned short usLength);

extern void SpiResumeSpi(void);

extern void CC3000InterruptHandler(void);

extern short SPIInterruptsEnabled;

//extern unsigned char wlan_tx_buffer[];
