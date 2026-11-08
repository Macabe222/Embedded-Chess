// Description: This file implements the initialization of an external
// switch.
//----------------------------------------------------------------------//

#include "switch.h"
#include <avr/io.h>

void initSwitchPE4(){
    DDRE &= ~(1 << PE4); // Set Pin B3 as input
    PORTE |= (1 << PE4); // Enable pull up resistor

    EICRB |=  (1 << ISC40);
    EICRB &= ~(1 << ISC41);
    EIMSK |=  (1 << INT4); // Select pin B3
}
