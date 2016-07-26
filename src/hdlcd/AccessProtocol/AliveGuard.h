/**
 * \file      AliveGuard.h
 * \brief     This file contains the header declaration of class AliveGuard
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright GNU Public License version 3.
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

#ifndef ALIVE_GUARD_H
#define ALIVE_GUARD_H

/*! \class AliveGuard
 *  \brief Class AliveGuard
 * 
 *  This guard object tracks whether a device attached via serial connections is alive, i.e., the baud rate is known and the HDLC protocol is initialized
 */
class AliveGuard {
public:
    /*! \brief The constructor of class AccessGuard
     * 
     *  On creation, a serial port is considered as alive
     */
    AliveGuard(): m_bAlive(true) {
    }
    
    /*! \brief Change the serial port state
     * 
     *  Change the serial port state
     * 
     * \param  a_bAlive the new state of the related serial device
     * \return bool indicates whether the state of the related serial device was changed by the call
     */
        
    bool UpdateSerialPortState(bool a_bAlive) {
        bool l_bStateChanged = (m_bAlive != a_bAlive);
        m_bAlive = a_bAlive;
        return l_bStateChanged;
    }

    /*! \brief Query whether the related serial port is currently alive
     * 
     *  Query whether the related serial device is currently alive
     * 
     * return bool indicating whether the related serial device is currently alive
     */
    bool IsAlive() const { return m_bAlive; }

private:
    // Members
    bool m_bAlive; //!< The alive state of the related device
};

#endif // ALIVE_GUARD_H
