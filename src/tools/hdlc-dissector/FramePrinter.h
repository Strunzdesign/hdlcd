/**
 * \file FramePrinter.h
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

#ifndef FRAME_PRINTER_H
#define FRAME_PRINTER_H

#include "../../libFrame/IBufferSink.h"
#include <iostream>
#include <string>

class FramePrinter: public IBufferSink {
private:
    void BufferReceived(E_DIRECTION a_eDirection, const std::vector<unsigned char> &a_Buffer) {
        // Print dissected HDLC frame
        std::string l_DissectedHDLCFrame;
        if (a_eDirection == DIRECTION_RCVD) {
            l_DissectedHDLCFrame = ">>> Rcvd ";
        } else {
            l_DissectedHDLCFrame = "<<< Sent ";
        } // else

        l_DissectedHDLCFrame.append((const char*)a_Buffer.data(), a_Buffer.size());
        std::cout << l_DissectedHDLCFrame << std::endl;
    }
};

#endif // FRAME_PRINTER_H
