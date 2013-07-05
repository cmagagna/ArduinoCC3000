ArduinoCC3000
=============

<p>Arduino library for the TI CC3000 WiFi module</p>

<p>Edit pin definitions etc. in ArduinoCC3000Core.h.</p>

<p>The .ino file demonstrates basic connectivity etc. For a full API
reference download the TI documentation at:</p>

http://processors.wiki.ti.com/index.php/CC3000

<pre>
/**************************************************************************
*
*  ArduinoCC3000.ino - Initial version of the Arduino CC3000 library.
*
*  Version 1.01
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

Changes since 1.0:

1. Fixed bug in TI's security.h where they missed a closing "}" so I had 
added it at the end of wlan.cpp (thanks to Frank Vannieuwkerke and alvarolb)

2. Fixed documentation about using INPUT_PULLUP on a 5V system (thanks to
Joseph Malkom)

3. Started going through TI's code and rewriting their use of 'longs' 
everywhere to 'bytes' or 'ints' to save RAM.

4. Added functionality to the demo program ArduinoCC3000.ino to demonstrate
Smart Config, DNS lookup, ping, etc.

5. Rewrote ArduinoCC3000Core.h to recognize Teensy 3.0 software SPI 
directly.

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
can read 3.3V signals directly. For WLAN_IRQ use a pullup resistor of 10K to
100K Ohm -- one leg to the Arduino input pin + CC3000 SPI_IRQ pin, the other
leg to +3.3V.

You can use a level shifter chip like the 74LVC245 or TXB0104 or you can use
a pair of resistors to make a voltage divider like this:

Arduino pin -----> 560 Ohm --+--> 1K Ohm -----> GND
                             |
                             |
                             +---> CC3000 pin

****************************************************************************/
</pre>
