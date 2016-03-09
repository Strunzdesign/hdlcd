/**
 * \file FlowGuard.h
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

#ifndef FLOW_GUARD_H
#define FLOW_GUARD_H

class FlowGuard {
public:
    // CTOR
    FlowGuard(): m_bFlowSuspended(false) {
    }
    
    // Influende the serial port state
    void UpdateSerialPortState(bool a_bFlowSuspended) {
        m_bFlowSuspended = a_bFlowSuspended;
    }
    
    // Check status
    bool IsFlowSuspended() const { return m_bFlowSuspended; }

private:
    // Members
    bool m_bFlowSuspended;
};

#endif // FLOW_GUARD_H
