// Description: This file implements functions that utilize the timers
//----------------------------------------------------------------------//

#include "timer.h"

/* Initialize timer 0, you should not turn the timer on here.
* You will need to use CTC mode */
void initTimer0(){
    TCCR0A = (1 << WGM01); // Initialize timer0 in CTC mode (allows it to compare)
    TCCR0B = (1 << CS02) | (1 << CS00); // Prescale by 1/1024
    TCCR0B &=  ~(1 << CS01);
    OCR0A = 155; // Timer now completes every 10 ms
    TIMSK0 = (1 << OCIE0A);
}

void initTimer1(){
    TCCR1B = (1 << WGM12) | (1 << CS10);
    OCR1A = 15;
}

void initTimer2(){
    TCCR2A = (1 << WGM21);   // CTC mode
    TCCR2B = (1 << CS22); // Prescale by 1/64
    OCR2A  = 249; // Timer now completes every 1 ms
}

void delayMs(unsigned int delay){
    // 1 ms per iteration
    for (unsigned int i = 0; i < delay; i++) {
        // Wait for the OCF2A flag to be set, signaling timer completion
        while (!(TIFR2 & (1 << OCF2A))){}

        // Clear flag so it can count up again next iteration
        TIFR2 |= (1 << OCF2A);
    }
}

void delayUs(unsigned int delay){

    // 1 us per iteration
    for (unsigned int i = 0; i < delay; i++) {
        // Wait for the OCF1A flag to be set, signaling timer completion
        while (!(TIFR1 & (1 << OCF1A))){}

        // Clear flag so it can count up again next iteration
        TIFR1 |= (1 << OCF1A);
    }
}
