#define SHIFTREGISTER_H
#ifndef SHIFTREGISTER_H

#include "avr/io.h"

void initSPI();
uint8_t readByte();
uint64_t readBoard();

#endif
