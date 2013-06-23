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


                                



extern void CC3000_Init(void);
