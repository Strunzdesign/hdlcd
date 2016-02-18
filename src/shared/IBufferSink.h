/**
 * \file IBufferSink.h
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

#ifndef I_BUFFER_SINK_H
#define I_BUFFER_SINK_H

#include "Direction.h"
#include <vector>

class IBufferSink {
public:
    virtual ~IBufferSink(){}
    virtual void BufferReceived(E_DIRECTION a_eDirection, const std::vector<unsigned char> &a_Buffer) = 0;
};

#endif // I_BUFFER_SINK_H
