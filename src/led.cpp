#include <avr/io.h>
#include <util/delay.h>
#include "led.h"


void initLED(){
    // Initialize pins as output
    DDRB |= (1 << PB7); // Green LED
    DDRC |= (1 << PC0); // Red LED
}

void turnOnGreenLED(){
    PORTB |= (1 << PB7); // Turn green on
    PORTC &= ~(1 << PC0); // Turn red off
}


void turnOnRedLED() {
    PORTC |= (1 << PC0); // Turn red on
    PORTB &= ~(1 << PB7); // Turn green off
}
