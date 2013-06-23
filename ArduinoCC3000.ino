/**************************************************************************
*
*  ArduinoCC3000.ino - An application to demo an Arduino connected to the
                       TI CC3000
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

You'll need to shift the pins for WLAN_CS, MOSI, SCK, and WLAN_EN. The other
2 pins (WLAN_IRQ and MISO) can be connected directly because they're input
pins for the Arduino, and the Arduino can read 3.3V signals directly.

You can use a level shifter chip like the 74LVC245 or TXB0104 or you can use
a pair of resistors to make a voltage divider like this:

Arduino pin -----> 560 Ohm -----> 1K Ohm -----> GND
                             |
                             |
                             +---> CC3000 pin


****************************************************************************/




#include <SPI.h>

#include "ArduinoCC3000Core.h"
#include "nvmem.h"
#include "socket.h"
#include "wlan.h"









void setup(void) {
	Serial.begin(9600);
	
	if (USE_HARDWARE_SPI) {
		SPI.begin();
		SPI.setDataMode(SPI_MODE1);
		SPI.setBitOrder(MSBFIRST);
		SPI.setClockDivider(SPI_CLOCK_DIV2);
		}
	else {
		// Faking SPI because the Teensy 3.0's hardware SPI library doesn't
		// seem to work with the CC3000
		pinMode(MOSI, OUTPUT);
		digitalWrite(MOSI, LOW);
		pinMode(SCK, OUTPUT);
		digitalWrite(SCK, LOW);
		pinMode(MISO, INPUT);	// Unlike a regular Arduino, the Teensy 3.0's pins default to 'disabled'
							    // instead of 'input', so we need to explicitly set this here
		}

	
	while (!Serial.available()) {
		Serial.println("Waiting on you before starting...");
		delay(1000);
		}
	Serial.read();
	
	Serial.println();
	
	Serial.print("Transmit buffer is ");
	Serial.print(CC3000_TX_BUFFER_SIZE);
	Serial.println(" bytes");
	
	Serial.print("Receive buffer is ");
	Serial.print(CC3000_RX_BUFFER_SIZE);
	Serial.println(" bytes");
	
	Serial.println();
	Serial.println("Initializing CC3000...");
	
	CC3000_Init();
	
	Serial.println("CC3000 is initialized");

	char fancyBuffer[128];
	nvmem_read_sp_version((unsigned char *)fancyBuffer);
	Serial.print("Version of the CC3000 is: ");
	Serial.print(fancyBuffer[0], DEC);
	Serial.print(".");
	Serial.println(fancyBuffer[1], DEC);
	
	/*Serial.println("Deleting all profiles...");
	byte rval = wlan_ioctl_del_profile(255);
	Serial.print("Got back: ");
	Serial.println(rval, HEX);
	
	Serial.println("Starting Smart Config");
	wlan_smart_config_start(1);*/
	
	}


	
void loop(void) {
	}