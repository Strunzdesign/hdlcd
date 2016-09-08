/**
 * \file      BaudRate.h
 * \brief     This file contains the header declaration of class BaudRate
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright GNU Public License version 3.
 *
 * The HDLC Deamon implements the HDLC protocol to easily talk to devices connected via serial communications.
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

/*! \class BaudRate
 *  \brief Class BaudRate
 * 
 *  This class is responsible for selecting and iterating through multiple pre-defined baud rate settings
 */
class BaudRate {
public:
    /*! \brief The constructor of BaudRate objects
     * 
     *  On creation, a rate of 9600 baud is selected
     */
    BaudRate(): m_CurrentBaudrateIndex(0) {}
    
    /*! \brief Deliver the currently used baud rate setting
     * 
     *  Deliver the currently used baud rate setting
     */
    unsigned int GetBaudRate() const {
        switch (m_CurrentBaudrateIndex) {
            case 0:
                return 9600;
            default:
            case 1:
                return 115200;
        } // switch
    }

    /*! \brief Iterate to another baud rate setting
     * 
     *  All available baud rates are used, then it starts over with the first setting
     */
    void ToggleBaudRate() {
        switch (m_CurrentBaudrateIndex) {
            case 0:
                m_CurrentBaudrateIndex = 1;
                break;
            default:
            case 1:
                m_CurrentBaudrateIndex = 0;
                break;
        } // switch
    }
    
private:
    unsigned int m_CurrentBaudrateIndex; //!< The current baud rate index
};

#endif // BAUD_RATE_H
