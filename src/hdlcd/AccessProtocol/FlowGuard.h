/**
 * \file      FlowGuard.h
 * \brief     This file contains the header declaration of class FlowGuard
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

#ifndef FLOW_GUARD_H
#define FLOW_GUARD_H

/*! \class FlowGuard
 *  \brief Class FlowGuard
 * 
 *  This guard object tracks whether a device attached via serial connections is suspended, i.e., one or more clients
 *  issued a "lock" on it to suspend exchange of HDLC PDUs.
 */
class FlowGuard {
public:
    /*! \brief The constructor of class FlowGuard
     * 
     *  On creation, a serial port is considered "unlocked" / resumed.
     */
    FlowGuard(): m_bFlowSuspended(false) {
    }
    
    /*! \brief Influende the suspend / resume state of a serial port
     * 
     *  Influende the suspend / resume state of a serial port
     * 
     *  \param a_bFlowSuspended the new state of the related serial device
     */
    void UpdateSerialPortState(bool a_bFlowSuspended) {
        m_bFlowSuspended = a_bFlowSuspended;
    }
    
    /*! \brief Query the suspend / resume state of a serial port
     * 
     *  Query the suspend / resume state of a serial port
     * 
     *  \return bool to indicate whether the data flow is currently suspended or resumed
     */
    bool IsFlowSuspended() const { return m_bFlowSuspended; }

private:
    // Members
    bool m_bFlowSuspended; //!< The state of the data flow of the related device
};

#endif // FLOW_GUARD_H
