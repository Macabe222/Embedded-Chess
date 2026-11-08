// Description: This file implements turnOnLED and initLED to control
// the LEDs.
//----------------------------------------------------------------------//


#include <avr/io.h>
#include <util/delay.h>
#include "led.h"


void initLED(){
    // Initialize pin as output
    DDRB |= (1 << PB7);
}

void turnOnLED(){
    PORTB |= (1 << PB7);
}
