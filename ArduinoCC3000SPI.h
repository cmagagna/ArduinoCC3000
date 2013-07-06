/**************************************************************************
*
*  ArduinoCC3000SPI.h - SPI functions to connect an Arduidno to the TI
*                       CC3000
*
*  This code uses the Arduino hardware SPI library (or a bit-banged
*  SPI for the Teensy 3.0) to send & receive data between the library
*  API calls and the CC3000 hardware. Every
*  
*  Version 1.0.1a
* 
*  Copyright (C) 2013 Chris Magagna - cmagagna@yahoo.com
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  Don't sue me if my code blows up your board and burns down your house
*
****************************************************************************/




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

extern unsigned char wlan_tx_buffer[];
