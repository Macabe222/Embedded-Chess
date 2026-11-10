#include "shiftregister.h"
#include "timer.h"

#define LOAD_BIT PB4 // Drives loading from sensors or shifting through registers
#define MISO PB3 // Serial output from the shift registers
#define MOSI PB2 // Serial input to the shift registers, not used but still set as output
#define SCK PB1 // Serial clock
#define SS PB0 // not actually used but we want to set it as an output so the microcontroller doesnt watch it

void initSPI() {
    // Load bit, SCK, and SS as outputs to the shift registers
    DDRB |= (1 << LOAD_BIT) | (1 << SCK) | (1 << SS) | (1 << MOSI);
    // MISO as input from the shift registers
    DDRB &= ~(1 << MISO);
    PORTB |= (1 << LOAD_BIT); // Load idles high, i.e. not loading

    // Enable SPI, set microcontroller to master, set clock to idle high,
    // first bit lands in bit 0, and clock frequency 250 kHz
    SPCR |= (1 << SPE) | (1 << MSTR) | (1 << CPOL) | (1 << DORD) | (1 << SPR1);

}

uint8_t readByte(){
    SPDR = 0x00;
    while (!(SPSR & (1 << SPIF))); // wait for byte to be written into SPDR
    return SPDR;
}

uint64_t readBoard() {
    uint64_t bits = 0;

    // Load sensor readings onto shift registers
    PORTB &= ~(1 << LOAD_BIT);
    delayUs(1);
    PORTB |= (1 << LOAD_BIT);

    for (uint8_t shiftCount = 0; shiftCount < 8; shiftCount++) {
        uint8_t occupied = ~readByte(); // Flip bits since sensors are connected to pull up resistors
        bits |= (uint64_t)occupied << (8 * shiftCount);
    }

    return bits;
}
