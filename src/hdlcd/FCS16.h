#ifndef FCS16_H
#define FCS16_H

#include <iostream>

#define PPPINITFCS16 0xffff
#define PPPGOODFCS16 0xf0b8

uint16_t pppfcs16(uint16_t fcs, unsigned char* cp, size_t len);

#endif // FCS16_H
