//The license: Its on the internet, have fun with it.

#ifndef DS1302_H
#define DS1302_H

#include "mbed.h"

/**
* Library to make use of the DS1302 timekeeping IC
*
* The library functions the same as the standard mbed time function,
* only now you have to first make a DS1302 object and apply the
* functions on the object.
*
* Example code:
* @code
* #define SCLK    PTC5
* #define IO      PTC4
* #define CE      PTC3
* 
* //Comment this line if the DS1302 is already running
* #define INITIAL_RUN
* 
* #include "mbed.h"
* #include "DS1302.h"
* 
* DS1302 clk(SCLK, IO, PTC3);
* 
* int main() {
*     #ifdef INITIAL_RUN
*     clk.set_time(1256729737);
*     #endif
*     
*     char storedByte = clk.recallByte(0);
*     printf("Stored byte was %d, now increasing by one\r\n", storedByte);
*     clk.storeByte(0, storedByte + 1);
*     
*     while(1) {
*         time_t seconds = clk.time(NULL);
*         printf("Time as a basic string = %s\r", ctime(&seconds));
*         wait(1);
*     }
* }
* @endcode
*
* See for example http://mbed.org/handbook/Time for general usage
* of C time functions. 
*
* Trickle charging is not supported
**/
class DS1302
{
public:
    /** 
    * Register map
    */
    enum {
        Seconds = 0x80,
        Minutes = 0x82,
        Hours = 0x84,
        Dates = 0x86,
        Months = 0x88,
        Days = 0x8A,
        Years = 0x8C,
        WriteProtect = 0x8E,
        Charge = 0x90,
        ClockBurst = 0xBE,
        RAMBase = 0xC0
    };

    /**
    * Create a new DS1302 object
    *
    * @param SCLK the pin to which SCLK is connectd
    * @param IO the pin to which IO is connectd
    * @param CE the pin to which CE is connected (also called RST)
    */
    DS1302(PinName SCLK, PinName IO, PinName CE);

    /** Set the current time
    *
    * Initialises and sets the time of the DS1302
    * to the time represented by the number of seconds since January 1, 1970
    * (the UNIX timestamp).
    *
    * @param t Number of seconds since January 1, 1970 (the UNIX timestamp)
    *
    */
    void set_time(time_t  t);

    /** Get the current time
    *
    * Use other functions to convert this value, see: http://mbed.org/handbook/Time
    *
    * @param t ignored, supply NULL
    * @return number of seconds since January 1, 1970
    */
    time_t time(time_t  *t = NULL);

    /**
    * Store a byte in the battery-backed RAM
    *
    * @param address address where to store the data (0-30)
    * @param data the byte to store
    */
    void storeByte(char address, char data);

    /**
    * Recall a byte from the battery-backed RAM
    *
    * @param address address where to retrieve the data (0-30)
    * @return the stored data
    */
    char recallByte(char address);

    /**
    * Read a register
    *
    * Only required to for example manually set trickle charge register
    *
    * @param reg register to read
    * @return contents of the register
    */
    char readReg(char reg);

    /**
    * Write a register
    *
    * Only required to for example manually set trickle charge register
    *
    * @param reg register to write
    * @param val contents of the register to write
    */
    void writeReg(char reg, char val);

protected:

    void writeByte(char data);
    char readByte(void);

    DigitalOut _SCLK;
    DigitalInOut _IO;
    DigitalOut _CE;
    bool writeProtect;


};



#endif
