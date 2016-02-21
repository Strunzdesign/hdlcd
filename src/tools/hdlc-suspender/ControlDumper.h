/**
 * \file ControlDumper.h
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

#ifndef CONTROL_DUMPER_H
#define CONTROL_DUMPER_H

#include "../../shared/IBufferSink.h"
#include <iostream>
#include <iomanip> 

class ControlDumper: public IBufferSink {
private:
    void BufferReceived(E_DIRECTION a_eDirection, const std::vector<unsigned char> &a_Buffer) {
        if (a_Buffer[0] == 0) {
            std::cout << "Serial port was resumed!" << std::endl;
        } else {
            std::cout << "Serial port was suspended: ";
            if (a_Buffer[1] == 0) {
                std::cout << "no own lock, ";
            } else {
                std::cout << "with own lock, ";
            } // else
            
            if (a_Buffer[2] == 0) {
                std::cout << "no other locks." << std::endl;
            } else {
                std::cout << "others have locks." << std::endl;
            } // else
        } // else
    }
};

#endif // CONTROL_DUMPER_H
