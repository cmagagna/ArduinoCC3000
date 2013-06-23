/**************************************************************************
*
*  ArduinoCC3000Core.cpp - Wrapper routines to make interfacing the Arduino
*                      and the TI CC3000 easier.
*
*  This code is based on the TI sample code "Basic WiFi Application"
*  and has the callback routines that TI expects for their library.
*
*  TI uses callback routines to make their library portable: these routines,
*  and the routines in the SPI files, will be different for an Arduino,
*  a TI MSP430, a PIC, etc. but the core library shouldn't have to be
*  changed.
*  
*  Version 1.0
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


#include <arduino.h>

#include "wlan.h"
#include "hci.h"
#include "ArduinoCC3000Core.h"
#include "ArduinoCC3000SPI.h"



#ifndef TEENSY3
#define digitalReadFast(pin)		digitalRead(pin)
#define digitalWriteFast(pin,state)	digitalWrite(pin,state)
#endif



volatile unsigned long ulSmartConfigFinished,
	ulCC3000Connected,
	ulCC3000DHCP,
	OkToDoShutDown,
	ulCC3000DHCP_configured;

volatile unsigned char ucStopSmartConfig;



#define NETAPP_IPCONFIG_MAC_OFFSET     (20)
#define CC3000_APP_BUFFER_SIZE         (5)
#define CC3000_RX_BUFFER_OVERHEAD_SIZE (20)

unsigned char pucCC3000_Rx_Buffer[CC3000_APP_BUFFER_SIZE + CC3000_RX_BUFFER_OVERHEAD_SIZE];









/*-------------------------------------------------------------------

    The TI library calls this routine when asynchronous events happen.
    
    For example you tell the CC3000 to turn itself on and connect
    to an access point then your code can go on to do its own thing.
    When the CC3000 is done configuring itself (e.g. it gets an IP
    address from the DHCP server) it will call this routine so you
    can take appropriate action.    

---------------------------------------------------------------------*/


void CC3000_AsyncCallback(long lEventType, char * data, unsigned char length) {

	switch (lEventType) {
  
		case HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE:
			ulSmartConfigFinished = 1;
			ucStopSmartConfig     = 1;
			Serial.println("CC3000 Async event: Simple config done"); 
			break;
			
		case HCI_EVNT_WLAN_UNSOL_CONNECT:
			ulCC3000Connected = 1;
			Serial.println("CC3000 Async event: Unsolicited connect");
			break;
			
		case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
			ulCC3000Connected = 0;
			ulCC3000DHCP      = 0;
			ulCC3000DHCP_configured = 0;
			Serial.println("CC3000 Async event: Unsolicted disconnect");
			break;
			
		case HCI_EVNT_WLAN_UNSOL_DHCP:
			// Notes: 
			// 1) IP config parameters are received swapped
			// 2) IP config parameters are valid only if status is OK, i.e. ulCC3000DHCP becomes 1
			// only if status is OK, the flag is set to 1 and the addresses are valid
			if ( *(data + NETAPP_IPCONFIG_MAC_OFFSET) == 0) {
				ulCC3000DHCP = 1;
				Serial.print("CC3000 Async event: Got IP address via DHCP: ");
				Serial.print(data[3], DEC); Serial.print(".");
				Serial.print(data[2], DEC); Serial.print(".");
				Serial.print(data[1], DEC); Serial.print(".");
				Serial.println(data[0], DEC);
				}
			else {
				ulCC3000DHCP = 0;
				Serial.println("CC3000 Async event: Unsolicited DHCP, no IP address");
				}
			break;
			
		case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
			OkToDoShutDown = 1;
			Serial.println("CC3000 Async event: OK to shut down");
			break;
		
		}
	}








/*-------------------------------------------------------------------

    The TI library calls these routines on CC3000 startup.
    
    This library does not send firmware, driver, or bootloader patches
    so we do nothing and we return NULL.

---------------------------------------------------------------------*/

char *SendFirmwarePatch(unsigned long *Length) {
	*Length = 0;
	return NULL;
	}



char *SendDriverPatch(unsigned long *Length) {
	*Length = 0;
	return NULL;
	}


char *SendBootloaderPatch(unsigned long *Length) {
	*Length = 0;
	return NULL;
	}









/*-------------------------------------------------------------------

    The TI library calls this routine to get the status of the
    WLAN_IRQ pin.
    
    The CC3000 will set this pin LOW when it needs to send data
    to the Arduino, and ordinarily the interrupt routine will
    be called via attachInterrupt(), but on some synchronous
    events (e.g. startup) the interrupt routine is disabled
    and the library reads it directly.

---------------------------------------------------------------------*/
long ReadWlanInterruptPin(void) {
	return(digitalReadFast(WLAN_IRQ));
	}











/*-------------------------------------------------------------------

    The TI library calls this routine to set the WLAN_EN pin,
    which tells the CC3000 to turn itself on (HIGH) or off (LOW).
    
 --------------------------------------------------------------------*/

void WriteWlanEnablePin( unsigned char val ) {

	if (val) {
		digitalWriteFast(WLAN_EN, HIGH);
		}
	else {
		digitalWriteFast(WLAN_EN, LOW);
		}
	}









/*-------------------------------------------------------------------

    The TI library calls these routines to enable or disable interrupts
    on the WLAN_IRQ pin.
    
    Originally WlanInterruptEnable() called attachInterrupt() and
    WlanInterruptDisable() called detachInterrupt() but the library
    was occationally locking up here, so now these routines just
    set a flag. The interrupt routine will always fire but if the
    flag isn't set it just returns immediately.
    
 --------------------------------------------------------------------*/

void WlanInterruptEnable(void) {
	SPIInterruptsEnabled = 1;
	}


void WlanInterruptDisable(void) {
	SPIInterruptsEnabled = 0;
	}











/*-------------------------------------------------------------------

    This is my routine to simplify CC3000 startup.
    
    It sets the Arduino pins then calls the normal CC3000 routines
    wlan_init() with all the callbacks and wlan_start() with 0
    to indicate we're not sending any patches.
    
 --------------------------------------------------------------------*/

void CC3000_Init(void) {

	SPIInterruptsEnabled = 0;
	pinMode(WLAN_IRQ, INPUT_PULLUP);
	attachInterrupt(WLAN_IRQ_INTNUM, CC3000InterruptHandler, FALLING);
	
	pinMode(WLAN_EN, OUTPUT);
	digitalWriteFast(WLAN_EN, LOW);		// make sure it's off until we're ready
	
	pinMode(WLAN_CS, OUTPUT);
	digitalWriteFast(WLAN_CS, HIGH);	// turn off CS until we're ready
	
	wlan_init( CC3000_AsyncCallback,
		SendFirmwarePatch,
		SendDriverPatch,
		SendBootloaderPatch,
		ReadWlanInterruptPin,
		WlanInterruptEnable,
		WlanInterruptDisable,
		WriteWlanEnablePin);
	
	wlan_start(0);
	}