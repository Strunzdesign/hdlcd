/**
 * \file BaudRate.h
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

#ifndef BAUD_RATE_H
#define BAUD_RATE_H

class BaudRate {
public:
    BaudRate(): m_CurrentBaudrateIndex(0) {}
    unsigned int GetNextBaudRate() {
        switch (m_CurrentBaudrateIndex) {
            case 0:
                m_CurrentBaudrateIndex = 1;
                return 9600;
            default:
            case 1:
                m_CurrentBaudrateIndex = 0;
                return 115200;
        } // switch
    }
    
private:
    size_t m_CurrentBaudrateIndex;
};

#endif // BAUD_RATE_H
