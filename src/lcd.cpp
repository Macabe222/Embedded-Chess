// Author:
// Net ID:
// Date:
// Assignment: Lab 3
//----------------------------------------------------------------------//
#include "lcd.h"
#include "timer.h"
#include <util/delay.h>

/*
 * Initializes all pins related to the LCD to be outputs
 */
void initLCDPins(){
    // Initialize control pins RS and EN as output
    DDRB |= (1 << PB6) | (1 << PB5);
    // Initialize data pins as output
    DDRA |= (1 << PA0) | (1 << PA1) | (1 << PA2) | (1 << PA3);
}



/* 1. Assert bits to pins connected to DB[7:4] on the LCD screen
 *      a. data is an unsigned char which has 8 bits. Therefore, you
 *         need assign the bottom 4 bits of "data" to the appropriate bits in
 *         PORTA
 *  2. This is a "command" signal, meaning RS should be Low
 *  3. Assert high on enable pin, delay, and asset low on enable pin
 *  4. delay the provided number in MICROseconds.
 */
void fourBitCommandWithDelay(unsigned char data, unsigned int delay){
  PORTA = 0x0F & data;
  // Set RS to low
  PORTB &= ~(1 << PB6);
  // Set EN to high
  PORTB |= (1 << PB5);
  // Delay to make sure signal reaches LCD
  delayUs(2);
  // Reset EN
  PORTB &= ~(1 << PB5);
  delayUs(delay);
}



/* Similar to fourBitCommandWithDelay except that now all eight bits of command are
 * sent.
 * 1. Assert bits to pins connected to DB[7:4] on the LCD screen
 *       a. unlike fourBitCommandWithDelay, you need to send the TOP four bits of
 *          "command" first. These should be assigned to appropriate bits in PORTA
 * 2. This is a command signal, meaning RS should be low
 * 3. Assert high on enable pin, delay, and asset low on enable pin
 * 4. Now set the lower four bits of command to appropriate bits in PORTA
 * 5. Assert high on enable pin, delay, and asset low on enable pin
 * 6. delay the provided number in MICROseconds.
 */
void eightBitCommandWithDelay(unsigned char command, unsigned int delay){
    // Top 4 bits
    PORTA = 0x0F & (command >> 4);
    // Set RS to low
    PORTB &= ~(1 << PB6);
    // Set EN to high
    PORTB |= (1 << PB5);
    // Delay to make sure signal reaches LCD
    delayUs(2);
    // Reset EN
    PORTB &= ~(1 << PB5);
    // Bottom 4 bits
    PORTA = 0x0F & command;
    // Set EN to high
    PORTB |= (1 << PB5);
    // Delay to make sure signal reaches LCD
    delayUs(2);
    // Reset EN
    PORTB &= ~(1 << PB5);
    delayUs(delay);
}



/* Similar to eightBitCommandWithDelay except that now RS should be high
 * 1. Assert bits to pins connected to DB[7:4] on the LCD screen
 * 2. This is a "data" signal, meaning RS should be high
 * 3. Assert high on enable pin, delay, and asset low on enable pin
 * 4. Now set the lower four bits of character to appropriate bits in PORTA
 * 5. Assert high on enable pin, delay, and asset low on enable pin
 * 6. delay is always 46 MICROseconds for a character write
 */
void writeCharacter(unsigned char character){
    // Top 4 bits
    PORTA = 0x0F & (character >> 4);
    // Set RS to high
    PORTB |= (1 << PB6);
    // Set EN to high
    PORTB |= (1 << PB5);
    // Delay to make sure signal reaches LCD
    delayUs(2);
    // Reset EN
    PORTB &= ~(1 << PB5);
    // Bottom 4 bits
    PORTA = 0x0F & character;
    // Set EN to high
    PORTB |= (1 << PB5);
    // Delay to make sure signal reaches LCD
    delayUs(2);
    // Reset EN
    PORTB &= ~(1 << PB5);
    delayUs(46);
}

/*
 * Writes a provided string such as "Hello!" to the LCD screen. You should
 * remember that a c string always ends with the '\0' character and
 * that this should just call writeCharacter multiple times.
 */
void writeString(const char *string){
    int i = 0;
    unsigned char cur;
    // Print character until null character
    while(1) {
        cur = string[i] ;
        if (cur == '\0') break;
        writeCharacter(cur);
        i++;
    }
}



/*
 * This moves the LCD cursor to a specific place on the screen.
 * This can be done using the eightBitCommandWithDelay with correct arguments
 */
void moveCursor(unsigned char x, unsigned char y){
    unsigned char address;
    // First row add offset to 0x80
    if (y == 0) {
        address = 0x80 + x;
    }
    // Second row add offset to 0xC0
    else {
        address = 0xC0 + x;
    }
    eightBitCommandWithDelay(address, 53);
}


/* This is the procedure outline on the LCD datasheet page 4 out of 9.
 * This should be the last function you write as it largely depends on all other
 * functions working.
 */
void initLCDProcedure(){
  // Delay 15 milliseconds
    delayMs(15);
  // Write 0b0011 to DB[7:4] and delay 4100 microseconds
    fourBitCommandWithDelay(0b0011, 4100);
  // Write 0b0011 to DB[7:4] and delay 100 microseconds
    fourBitCommandWithDelay(0b0011, 100);
  // The data sheet does not make this clear, but at this point you are issuing
  // commands in two sets of four bits. You must delay after each command
  // (which is the second set of four bits) an amount specified on page 3 of
  // the data sheet.
  // write 0b0011 to DB[7:4] and 100us delay
    fourBitCommandWithDelay(0b0011, 100);
  // write 0b0010 to DB[7:4] and 100us delay.
    fourBitCommandWithDelay(0b0010, 100);
  // Function set in the command table with 53us delay
    eightBitCommandWithDelay(0b00101000, 53);
  // Display off in the command table with 53us delay
    eightBitCommandWithDelay(0b00001000, 53);
  // Clear display in the command table. Remember the delay is longer!!!
    eightBitCommandWithDelay(0b00000001, 2000);
  // Entry Mode Set in the command table.
    eightBitCommandWithDelay(0b00000110, 53);
  // Display ON/OFF Control in the command table. (Yes, this is not specified),
  // in the data sheet, but you have to do it to get this to work. Yay datasheets!)
    eightBitCommandWithDelay(0b00001100, 53);
}



/* Initializes Tri-state for LCD pins and calls initialization procedure.
* This function is made so that it's possible to test initLCDPins separately
* from initLCDProcedure which will likely be necessary.
*/
void initLCD(){
  initLCDPins();
  initLCDProcedure();
}
