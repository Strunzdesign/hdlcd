/**
 * \file HexDumper.h
 * \brief 
 *
 * The hdlc-tools implement the HDLC protocol to easily talk to devices connected via serial communications
 * Copyright (C) 2016  Florian Evers, florian-evers@gmx.de
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HEX_DUMPER_H
#define HEX_DUMPER_H

#include <iostream>
#include <iomanip> 

void PrintHexDump(bool a_bWasSent, const std::vector<unsigned char> &a_Buffer) {
    // Print a hexdump of the provided data buffer. It should contain a packet to be printed in one line.
    if (a_bWasSent) {
        std::cout << "<<< Sent: ";
    } else {
        std::cout << ">>> Rcvd: ";
    } // else

    for (auto it = a_Buffer.begin(); it != a_Buffer.end(); ++it) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << int(*it) << " ";
    } // for

    std::cout << std::endl;
}

#endif // HEX_DUMPER_H
