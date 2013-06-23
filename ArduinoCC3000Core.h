// These are the Arduino pins that connect to the CC3000 
// (in addition to standard SPI pins MOSI, MISO, and SCK)
//
// The WLAN_IRQ pin must be supported by attachInterrupt
// on your platform


#define WLAN_CS			10		// Arduino pin connected to CC3000 WLAN_SPI_CS

#define WLAN_EN			9		// Arduino pin connected to CC3000 VBAT_SW_EN

#define WLAN_IRQ		3		// Arduino pin connected to CC3000 WLAN_SPI_IRQ

#define WLAN_IRQ_INTNUM	1		// The attachInterrupt() number that corresponds
                                // to WLAN_IRQ

#ifndef MISO
#define MISO 12
#endif

#ifndef MOSI
#define MOSI 11
#endif

#ifndef SCK
#define SCK 13
#endif
                                
                                

/* If this flag is defined then the Teensy optimizations to use

   digitalWriteFast() instead of digitalWrite()
   digitalReadFast() instead of digitalRead()
   
   will be substituted
*/

//#define TEENSY3   




/* I used the Teensy 3.0 to get the Arduino CC3000 library working but the
   Teensy's hardware SPI and the CC3000's SPI didn't like each other so I had
   to send the bits manually. For the Uno, Nano, etc. you can probably leave
   this unchanged. If your Arduino can't talk to the CC3000 and you're sure
   your wiring is OK then try changing this. */
   
#define USE_HARDWARE_SPI	true





/* If you uncomment the line below the library will leave out a lot of the
   higher level functions but use a lot less memory. From:
   
   http://processors.wiki.ti.com/index.php/Tiny_Driver_Support
   
  CC3000's new driver has flexible memory compile options.
  
  This feature comes in handy when we want to use a limited RAM size MCU.
  
  Using The Tiny Driver Compilation option will create a tiny version of our
  host driver with lower data, stack and code consumption.
  
  By enabling this feature, host driver's RAM consumption can be reduced to
  minimum of 251 bytes.
  
  The Tiny host driver version will limit the host driver API to the most
  essential ones.
  
  Code size depends on actual APIs used.
  
  RAM size depends on the largest packet sent and received.
  
  CC3000 can now be used with ultra low cost MCUs, consuming 251 byte of RAM
  and 2K to 6K byte of code size, depending on the API usage. */

//#define CC3000_TINY_DRIVER	1



                                



extern void CC3000_Init(void);
