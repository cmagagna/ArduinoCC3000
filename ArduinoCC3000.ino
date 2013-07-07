/**************************************************************************
*
*  ArduinoCC3000.ino - An application to demo an Arduino connected to the
                       TI CC3000
*
*  Version 1.0.1b
*
*  Copyright (C) 2013 Chris Magagna - cmagagna@yahoo.com
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*  Don't sue me if my code blows up your board and burns down your house
*
****************************************************************************



To connect an Arduino to the CC3000 you'll need to make these 6 connections
(in addition to the WiFi antenna, power etc).

Name / pin on CC3000 module / pin on CC3000EM board / purpose

SPI_CS     / 12 / J4-8 /  SPI Chip Select
                          The Arduino will set this pin LOW when it wants to
                          exchange data with the CC3000. By convention this is
                          Arduino pin 10, but any pin can be used. In this
                          program it will be called WLAN_CS

SPI_DOUT   / 13 / J4-9 /  Data from the module to the Arduino
                          This is Arduino's MISO pin, and is how the CC3000
                          will get bytes to the Arduino. For most Arduinos
                          MISO is pin 12

SPI_IRQ    / 14 / J4-10 / CC3000 host notify
                          The CC3000 will drive this pin LOW to let the Arduino
                          know it's ready to send data. For a regular Arduino
                          (Uno, Nano, Leonardo) this will have to be connected
                          to pin 2 or 3 so you can use attachInterrupt(). In
                          this program it will be called WLAN_IRQ

SPI_DIN    / 15 / J4-11   Data from the Arduino to the CC3000
                          This is the Arduino's MOSI pin, and is how the Arduino
                          will get bytes to the CC3000. For most Arduinos
                          MOSI is pin 11

SPI_CLK    / 17 / J4-12   SPI clock
                          This is the Arduino's SCK pin. For most Arduinos
                          SCK is pin 13

VBAT_SW_EN / 26 / J5-5    Module enable
                          The Arduino will set this pin HIGH to turn the CC3000
                          on. Any pin can be used. In this program it will be
                          called WLAN_EN


WARNING #1: The CC3000 runs at 3.6V maximum so you can't run it from your
regular 5V Arduino power pin. Run it from 3.3V!


WARNING #2: When transmitting the CC3000 will use up to 275mA current. Most
Arduinos' 3.3V pins can only supply up to 50mA current, so you'll need a
separate power supply for it (or a voltage regulator like the LD1117V33
connected to your Arduino's 5V power pin).


WARNING #3: The CC3000's IO pins are not 5V tolerant. If you're using a 5V
Arduino you will need a level shifter to convert these signals to 3.3V
so you don't blow up the module.

You'll need to shift the pins for WLAN_CS, MOSI, SCK, and WLAN_EN. MISO can be
connected directly because it's an input pin for the Arduino and the Arduino
can read 3.3V signals directly. For WLAN_IRQ use a pullup resistor of 20K to
100K Ohm -- one leg to the Arduino input pin + CC3000 SPI_IRQ pin, the other
leg to +3.3V.

You can use a level shifter chip like the 74LVC245 or TXB0104 or you can use
a pair of resistors to make a voltage divider like this:

Arduino pin -----> 560 Ohm --+--> 1K Ohm -----> GND
                             |
                             |
                             +---> CC3000 pin


****************************************************************************/




#include <SPI.h>

#include "ArduinoCC3000Core.h"
#include "nvmem.h"
#include "socket.h"
#include "wlan.h"
#include "hci.h"
#include "security.h"
#include "netapp.h"




// When operations that take a long time (like Smart Config) are running, the
// function Blinker() flashes this LED. It's not required for actual use.

#define BLINKER_LED	6




byte isInitialized = false;








void AsyncEventPrint(void) {
	switch(lastAsyncEvent) {
			Serial.println(F("CC3000 Async event: Simple config done"));
			break;

		case HCI_EVNT_WLAN_UNSOL_CONNECT:
			Serial.println(F("CC3000 Async event: Unsolicited connect"));
			break;

		case HCI_EVNT_WLAN_UNSOL_DISCONNECT:
			Serial.println(F("CC3000 Async event: Unsolicted disconnect"));
			break;

		case HCI_EVNT_WLAN_UNSOL_DHCP:
			Serial.print(F("CC3000 Async event: Got IP address via DHCP: "));
			Serial.print(dhcpIPAddress[0]);
			Serial.print(F("."));
			Serial.print(dhcpIPAddress[1]);
			Serial.print(F("."));
			Serial.print(dhcpIPAddress[2]);
			Serial.print(F("."));
			Serial.println(dhcpIPAddress[3]);
			break;

		case HCI_EVENT_CC3000_CAN_SHUT_DOWN:
			Serial.println(F("CC3000 Async event: OK to shut down"));
			break;

		case HCI_EVNT_WLAN_KEEPALIVE:
			// Once initialized, the CC3000 will send these keepalive events
			// every 20 seconds.
			//Serial.println(F("CC3000 Async event: Keepalive"));
			return;
			break;

		default:
			Serial.print(F("AsyncCallback called with unhandled event! ("));
			Serial.print(lastAsyncEvent, HEX);
			Serial.println(F(")"));
			break;
		}
		
	Serial.println();
	}







void setup(void) {
	Serial.begin(115200);

	pinMode(BLINKER_LED, OUTPUT);

	if (USE_HARDWARE_SPI) {
		SPI.begin();
		SPI.setDataMode(SPI_MODE1);
		SPI.setBitOrder(MSBFIRST);
		SPI.setClockDivider(SPI_CLOCK_DIV2);
		}
	else {
		// Faking SPI because the Teensy 3.0's hardware SPI library doesn't
		// seem to work with the CC3000
		pinMode(WLAN_MOSI, OUTPUT);
		digitalWrite(WLAN_MOSI, LOW);
		pinMode(WLAN_SCK, OUTPUT);
		digitalWrite(WLAN_SCK, LOW);
		pinMode(WLAN_MISO, INPUT);	// Unlike a regular Arduino, the Teensy 3.0's pins default to 'disabled'
									// instead of 'input', so we need to explicitly set this here
		}

	}



void loop(void) {
	char cmd;

	Serial.println();
	Serial.println(F("+-------------------------------------------+"));
	Serial.println(F("|      Arduino CC3000 Demo Program          |"));
	Serial.println(F("+-------------------------------------------+"));
	Serial.println();
	Serial.println(F("  1 - Initialize the CC3000"));
	Serial.println(F("  2 - Show RX & TX buffer sizes, & free RAM"));
	Serial.println(F("  3 - Start Smart Config"));
	Serial.println(F("  4 - Manually connect to AP"));
	Serial.println(F("  5 - Manually add connection profile"));
	Serial.println(F("  6 - List access points"));
	Serial.println(F("  7 - Show CC3000 information"));
	Serial.println();

	for (;;) {
		while (!Serial.available()) {
			if (asyncNotificationWaiting) {
				asyncNotificationWaiting = false;
				AsyncEventPrint();
				}
			}
		cmd = Serial.read();
		if (cmd!='\n' && cmd!='\r') {
			break;
			}
		}

	switch(cmd) {
		case '1':
			Initialize();
			break;
		case '2':
			ShowBufferSize();
			ShowFreeRAM();
			break;
		case '3':
			StartSmartConfig();
			break;
		case '4':
			ManualConnect();
			break;
		case '5':
			ManualAddProfile();
			break;
		case '6':
			ListAccessPoints();
			break;
		case '7':
			ShowInformation();
			break;
		default:
			Serial.print(F("**Unknown command \""));
			Serial.print(cmd);
			Serial.println(F("\"**"));
			break;
		}

	}














// 



void Initialize(void) {

	unsigned char fancyBuffer[MAC_ADDR_LEN];

	if (isInitialized) {
		Serial.println(F("CC3000 already initialized. Shutting down and restarting..."));
		wlan_stop();
		delay(1000);
		}

	Serial.println(F("Initializing CC3000..."));
	CC3000_Init();
	Serial.println(F("  CC3000 init complete."));

	if (nvmem_read_sp_version(fancyBuffer)==0) {
		Serial.print(F("  Firmware version is: "));
		Serial.print(fancyBuffer[0], DEC);
		Serial.print(F("."));
		Serial.println(fancyBuffer[1], DEC);
		}
	else {
		Serial.println(F("Unable to get firmware version. Can't continue."));
		return;
		}
	
	if (nvmem_get_mac_address(fancyBuffer)==0) {
		Serial.print(F("  MAC address: "));
		for (byte i=0; i<MAC_ADDR_LEN; i++) {
			if (i!=0) {
				Serial.print(F(":"));
				}
			Serial.print(fancyBuffer[i], HEX);
			}
		Serial.println();
		
		isInitialized=true;
		}
	else {
		Serial.println(F("Unable to get MAC address. Can't continue."));
		}
	
	}













/* This just shows the compiled size of the transmit & recieve buffers */

void ShowBufferSize(void) {
	Serial.print(F("Transmit buffer is "));
	Serial.print(CC3000_TX_BUFFER_SIZE);
	Serial.println(F(" bytes"));

	Serial.print(F("Receive buffer is "));
	Serial.print(CC3000_RX_BUFFER_SIZE);
	Serial.println(F(" bytes"));
	}





int freeRam () {

#if defined(TEENSY3)
/* This routine from:
	https://github.com/madsci1016/Sparkfun-MP3-Player-Shield-Arduino-Library/blob/master/SdFat/SdFatUtil.cpp
*/
	extern char *__brkval;
	char top;
	return &top - __brkval;
#else

/* This routine from:
	http://www.controllerprojects.com/2011/05/23/determining-sram-usage-on-arduino/
*/

  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);

#endif
	}

void ShowFreeRAM(void) {
	Serial.print(F("Free RAM is: "));
	Serial.print(freeRam());
	Serial.println(F(" bytes"));
	}










void Blinker(void) {
	digitalWrite(BLINKER_LED, HIGH);
	delay(50);
	digitalWrite(BLINKER_LED, LOW);
	delay(250);
	}










/*
	Smart Config is TI's way to let you connect your device to your WiFi network
	without needing a keyboard and display to enter the network name, password,
	etc. You run a little app on your iPhone, Android device, or laptop with Java
	and it sends the config info to the CC3000 automagically, so the end user
	doesn't need to do anything complicated. More details here:
	
		http://processors.wiki.ti.com/index.php/CC3000_Smart_Config
		
	This example deletes any currently saved WiFi profiles and goes over the top
	with error checking, so it's easier to see exactly what's going on. You
	probably won't need all of this code for your own Smart Config implementation.
	
	This example also doesn't use any of the AES enhanced security setup API calls
	because frankly they're weirder than I want to deal with.
*/


// The Simple Config Prefix always needs to be 'TTT'
char simpleConfigPrefix[] = {'T', 'T', 'T'};

// This is the default Device Name that TI's Smart Config app for iPhone etc. use.
// You can change it to whatever you want, but then your users will need to type
// that name into their phone or tablet when they run Smart Config.
char device_name[]	= "CC3000";

void StartSmartConfig(void) {
	long rval;
	long timeoutCounter;

	if (!isInitialized) {
		Serial.println(F("CC3000 not initialized; can't run Smart Config."));
		return;
		}

	Serial.println(F("Starting Smart Config..."));

	Serial.println(F("  Disabling auto-connect policy..."));
	if ((rval = wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE))!=0) {
		Serial.print(F("    Setting auto connection policy failed, error: "));
		Serial.println(rval, HEX);
		return;
		}
		
	Serial.println(F("  Deleting all existing profiles..."));
	if ((rval = wlan_ioctl_del_profile(255))!=0) {
		Serial.print(F("    Deleting all profiles failed, error: "));
		Serial.println(rval, HEX);
		return;
		}

	Serial.println(F("  Waiting until disconnected..."));
	while (ulCC3000Connected == 1) {
		Blinker();
		}

	Serial.println(F("  Setting smart config prefix..."));
	if ((rval = wlan_smart_config_set_prefix(simpleConfigPrefix))!=0) {
		Serial.print(F("    Setting smart config prefix failed, error: "));
		Serial.println(rval, HEX);
		return;
		}

	Serial.println(F("  Starting smart config..."));
	if ((rval = wlan_smart_config_start(0))!=0) {
		Serial.print(F("    Starting smart config failed, error: "));
		Serial.println(rval, HEX);
		return;
		}

	// Wait for Smartconfig process complete, or 30 seconds, whichever
	// comes first. The Uno isn't seeing the HCI_EVNT_WLAN_ASYNC_SIMPLE_CONFIG_DONE
	// event and I can't figure out why (it works fine on the Teensy) so my
	// temporary workaround is I just stop waiting after a while
	timeoutCounter=millis();
	while (ulSmartConfigFinished == 0)	{
		Blinker();
		if (millis() - timeoutCounter > 30000) {
			Serial.println(F("    Timed out waiting for Smart Config to finish. Hopefully it did anyway"));
			break;
			}
		}

	Serial.println(F("  Smart Config packet seen!"));

	Serial.println(F("  Enabling auto-connect policy..."));
	if ((rval=wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE))!=0) {
		Serial.print(F("    Setting auto connection policy failed, error: "));
		Serial.println(rval, HEX);
		return;
		}

	Serial.println(F("  Stopping CC3000..."));
	wlan_stop();	// no error returned here, so nothing to check

	Serial.println(F("  Pausing for 2 seconds..."));
	delay(2000);

	Serial.println(F("  Restarting CC3000... "));
	wlan_start(0);	// no error returned here, so nothing to check

	Serial.println(F("  Waiting for connection to AP..."));
	while (ulCC3000Connected!=1) {
		Blinker();
		}
		
	Serial.println(F("  Waiting for IP address from DHCP..."));
	while (ulCC3000DHCP!=1) {
		Blinker();
		}
		
	Serial.println(F("  Sending mDNS broadcast to signal we're done with Smart Config..."));
	mdnsAdvertiser(1,device_name,strlen(device_name)); // The API documentation says mdnsAdvertiser()
			// is supposed to return 0 on success and SOC_ERROR on failure, but it looks like
			// what it actually returns is the socket number it used. So we ignore it.

	Serial.println(F("  Smart Config finished!"));
	}














/*
	This is an example of how you'd connect the CC3000 to an AP without using
	Smart Config or a stored profile.
	
	All the code above wlan_connect() is just for this demo program; if you're
	always going to connect to your network this way you wouldn't need it.
*/

void ManualConnect(void) {

	char ssidName[] = "PotatoTron";
	char AP_KEY[] = "cromulent";
	byte rval;

	if (!isInitialized) {
		Serial.println(F("CC3000 not initialized; can't run manual connect."));
		return;
		}

	Serial.println(F("Starting manual connect..."));
	
	Serial.println(F("  Disabling auto-connect policy..."));
	rval = wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);

	Serial.println(F("  Deleting all existing profiles..."));
	rval = wlan_ioctl_del_profile(255);

	Serial.println(F("  Waiting until disconnected..."));
	while (ulCC3000Connected == 1) {
		delay(100);
		}
	
	Serial.println(F("  Manually connecting..."));

	// Parameter 1 is the security type: WLAN_SEC_UNSEC, WLAN_SEC_WEP,
	//				WLAN_SEC_WPA or WLAN_SEC_WPA2
	// Parameter 3 is the MAC adddress of the AP. All the TI examples
	//				use NULL. I suppose you would want to specify this
	//				if you were security paranoid.
	rval = wlan_connect(WLAN_SEC_WPA2,
				ssidName,
				strlen(ssidName),
				NULL,
				(unsigned char *)AP_KEY,
				strlen(AP_KEY));

	if (rval==0) {
		Serial.println(F("  Manual connect success."));
		}
	else {
		Serial.print(F("  Unusual return value: "));
		Serial.println(rval);
		}
	}



















/*
	This is an example of manually adding a WLAN profile to the CC3000. See
	wlan_ioctl_set_connection_policy() for more details of how profiles are
	used but basically there's 7 slots where you can store AP info and if
	the connection policy is set to auto_start then the CC3000 will go
	through its profile table and try to auto-connect to something it knows
	about after it boots up.
	
	Note the API documentation for wlan_add_profile is wrong. It says it
	returns 0 on success and -1 on failure. What it really returns is
	the stored profile number (0-6, since the CC3000 can store 7) or
	255 on failure.
	
	Unfortunately the API doesn't give you any way to see how many profiles
	are in use or which profile is stored in which slot, so if you want to
	manage multiple profiles you'll need to do that yourself.
*/

void ManualAddProfile(void) {
	char ssidName[] = "PotatoTron";
	char AP_KEY[] = "cromulent";

	if (!isInitialized) {
		Serial.println(F("CC3000 not initialized; can't run manual add profile."));
		return;
		}

	Serial.println(F("Starting manual add profile..."));

	Serial.println(F("  Disabling auto connection..."));
	wlan_ioctl_set_connection_policy(DISABLE, DISABLE, DISABLE);

	Serial.println("  Adding profile...");
	byte rval = wlan_add_profile  (
					WLAN_SEC_WPA2,		// WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
					(unsigned char *)ssidName,
					strlen(ssidName),
					NULL,				// BSSID, TI always uses NULL
					0,					// profile priority
					0x18,				// key length for WEP security, undocumented why this needs to be 0x18
					0x1e,				// key index, undocumented why this needs to be 0x1e
					0x2,				// key management, undocumented why this needs to be 2
					(unsigned char *)AP_KEY,	// WPA security key
					strlen(AP_KEY)		// WPA security key length
					);

	if (rval!=255) {

		// This code is lifted from http://e2e.ti.com/support/low_power_rf/f/851/p/180859/672551.aspx;
		// the actual API documentation on wlan_add_profile doesn't specify any of this....

		Serial.print(F("  Manual add profile success, stored in profile: "));
		Serial.println(rval, DEC);

		Serial.println(F("  Enabling auto connection..."));
		wlan_ioctl_set_connection_policy(DISABLE, DISABLE, ENABLE);

		Serial.println(F("  Stopping CC3000..."));
		wlan_stop();

		Serial.println(F("  Stopping for 5 seconds..."));
		delay(5000);

		Serial.println(F("  Restarting CC3000..."));
		wlan_start(0);
		
		Serial.println(F("  Manual add profile done!"));

		}
	else {
		Serial.print(F("  Manual add profile failured (all profiles full?)."));
		}
	}
	








	










	
/*
	The call wlan_ioctl_get_scan_results returns this structure. I couldn't
	find it in the TI library so it's defined here. It's 50 bytes with
	a semi weird arrangement but fortunately it's not as bad as it looks.
	
	numNetworksFound - 4 bytes - On the first call to wlan_ioctl_get_scan_results
					this will be set to how many APs the CC3000 sees. Although
					with 4 bytes the CC3000 could see 4 billion APs in my testing
					this number was always 20 or less so there's probably an
					internal memory limit.
	
	results - 4 bytes - 0=aged results, 1=results valid, 2=no results. Why TI
					used 32 bits to store something that could be done in 2,
					and how this field is different than isValid below, is
					a mystery to me so I just igore this field completely.
					
	isValid & rssi - 1 byte - a packed structure. The top bit (isValid)
					indicates whether or not this structure has valid data,
					the bottom 7 bits (rssi) are the signal strength of this AP.
					
	securityMode & ssidLength - 1 byte - another packed structure. The top 2
					bits (securityMode) show how the AP is configured:
						0 - open / no security
						1 - WEP
						2 - WPA
						3 - WPA2
					ssidLength is the lower 6 bytes and shows how many characters
					(up to 32) of the ssid_name field are valid
					
	frameTime - 2 bytes - how long, in seconds, since the CC3000 saw this AP
					beacon
					
	ssid_name - 32 bytes - The ssid name for this AP. Note that this isn't a
					regular null-terminated C string so you can't use it
					directly with a strcpy() or Serial.println() etc. and you'll
					need a 33-byte string to store it (32 valid characters +
					null terminator)
					
	bssid - 6 bytes - the MAC address of this AP
*/
	
typedef struct scanResults {
	unsigned long numNetworksFound;
	unsigned long results;
	unsigned isValid:1;
	unsigned rssi:7;
	unsigned securityMode:2;
	unsigned ssidLength:6;
	unsigned short frameTime;
	unsigned char ssid_name[32];
	unsigned char bssid[6];
	} scanResults;

#define NUM_CHANNELS	16

void ListAccessPoints(void) {
	unsigned long aiIntervalList[NUM_CHANNELS];
	byte rval;
	scanResults sr;
	int apCounter;
	char localB[33];
	
	if (!isInitialized) {
		Serial.println(F("CC3000 not initialized; can't list access points."));
		return;
		}
		
	Serial.println(F("List visible access points"));
	
	Serial.println(F("  Setting scan paramters..."));
	
	for (int i=0; i<NUM_CHANNELS; i++) {
		aiIntervalList[i] = 2000;
		}
	
	rval = wlan_ioctl_set_scan_params(
			1000,	// enable start application scan
			100,	// minimum dwell time on each channel
			100,	// maximum dwell time on each channel
			5,		// number of probe requests
			0x7ff,	// channel mask
			-80,	// RSSI threshold
			0,		// SNR threshold
			205,	// probe TX power
			aiIntervalList	// table of scan intervals per channel
			);
	if (rval!=0) {
		Serial.print(F("  Got back unusual result from wlan_ioctl_set_scan_params, can't continue: "));
		Serial.println(rval);
		return;
		}
		
	Serial.println(F("  Sleeping 5 seconds to let the CC3000 discover APs..."));
	delay(5000);
	
	Serial.println(F("  Getting AP count..."));
	
	// On the first call to get_scan_results, sr.numNetworksFound will return the
	// actual # of APs currently seen. Get that # then loop through and print
	// out what's found.
	
	if ((rval=wlan_ioctl_get_scan_results(2000, (unsigned char *)&sr))!=0) {
		Serial.print(F("  Got back unusual result from wlan_ioctl_get scan results, can't continue: "));
		Serial.println(rval);
		return;
		}
		
	apCounter = sr.numNetworksFound;
	Serial.print(F("  Number of APs found: "));
	Serial.println(apCounter);
	
	do {
		if (sr.isValid) {
			Serial.print(F("    "));
			switch(sr.securityMode) {
				case WLAN_SEC_UNSEC:	// 0
					Serial.print(F("OPEN "));
					break;
				case WLAN_SEC_WEP:		// 1
					Serial.print(F("WEP  "));
					break;
				case WLAN_SEC_WPA:		// 2
					Serial.print(F("WPA  "));
					break;
				case WLAN_SEC_WPA2:		// 3
					Serial.print(F("WPA2 "));
					break;
					}
			sprintf(localB, "%3d  ", sr.rssi);
			Serial.print(localB);
			memset(localB, 0, 33);
			memcpy(localB, sr.ssid_name, sr.ssidLength);
			Serial.println(localB);
			}
			
		if (--apCounter>0) {
			if ((rval=wlan_ioctl_get_scan_results(2000, (unsigned char *)&sr))!=0) {
				Serial.print(F("  Got back unusual result from wlan_ioctl_get scan results, can't continue: "));
				Serial.println(rval);
				return;
				}
			}
		} while (apCounter>0);
		
	Serial.println(F("  Access Point list finished."));
	}



















void PrintIPBytes(unsigned char *ipBytes) {
	Serial.print(ipBytes[3]);
	Serial.print(F("."));
	Serial.print(ipBytes[2]);
	Serial.print(F("."));
	Serial.print(ipBytes[1]);
	Serial.print(F("."));
	Serial.println(ipBytes[0]);
	}


/*
	All the data in all the fields from netapp_ipconfig() are reversed,
	e.g. an IP address is read via bytes 3,2,1,0 instead of bytes
	0,1,2,3 and the MAC address is read via bytes 5,4,3,2,1,0 instead
	of 0,1,2,3,4,5.
	
	N.B. TI is inconsistent here; nvmem_get_mac_address() returns them in
	the right order etc.
*/

void ShowInformation(void) {

	tNetappIpconfigRetArgs inf;
	char localB[33];

	if (!isInitialized) {
		Serial.println(F("CC3000 not initialized; can't get information."));
		return;
		}
		
	Serial.println(F("CC3000 information:"));
	
	netapp_ipconfig(&inf);
	
	Serial.print(F("  IP address: "));
	PrintIPBytes(inf.aucIP);
	
	Serial.print(F("  Subnet mask: "));
	PrintIPBytes(inf.aucSubnetMask);
	
	Serial.print(F("  Gateway: "));
	PrintIPBytes(inf.aucDefaultGateway);
	
	Serial.print(F("  DHCP server: "));
	PrintIPBytes(inf.aucDHCPServer);
	
	Serial.print(F("  DNS server: "));
	PrintIPBytes(inf.aucDNSServer);
	
	Serial.print(F("  MAC address: "));
	for (int i=(MAC_ADDR_LEN-1); i>=0; i--) {
		if (i!=(MAC_ADDR_LEN-1)) {
			Serial.print(F(":"));
			}
		Serial.print(inf.uaMacAddr[i], HEX);
		}
	Serial.println();
	
	memset(localB, 0, 32);
	memcpy(localB, inf.uaSSID, 32);

	Serial.print(F("  Connected to SSID: "));
	Serial.println(localB);

	}