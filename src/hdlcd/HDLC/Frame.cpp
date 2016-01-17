/**
 * \file Frame.cpp
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

#include "Frame.h"
#include <sstream>

std::string Frame::GetReadableDescription() const {
    std::stringstream l_Output;
    if (IsIFrame()) {
        l_Output << "HDLC frame, Addr=" << (int)GetAddress() << ", ";
        l_Output << "I-Frame, PF=" << IsPF() << ", SSeq=" << (int)GetSSeq() << ", RSeq=" << (int)GetRSeq();
    } else if (IsSFrame()) {
        l_Output << "HDLC frame, Addr=" << (int)GetAddress() << ", ";
        l_Output << "S-Frame, PF=" << IsPF() << ", RSeq=" << (int)GetRSeq();
    } else if (IsUFrame()) {
        l_Output << "HDLC frame, Addr=" << (int)GetAddress() << ", ";
        l_Output << "U-Frame, PF=" << IsPF();
    } else {
        l_Output << "Unparseable HDLC frame";
    } // else

    return l_Output.str();
}
