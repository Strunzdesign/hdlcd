/**
 * \file PacketCtrl.h
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

#ifndef PACKET_CTRL_H
#define PACKET_CTRL_H

#include "Packet.h"

class PacketCtrl: public Packet {
public:
    // Types of control packets
    typedef enum {
        CTRL_TYPE_PORT_STATUS = 0x00,
        CTRL_TYPE_ECHO        = 0x10,
        CTRL_TYPE_KEEP_ALIVE  = 0x20,
        CTRL_TYPE_PORT_KILL   = 0x30,
    } E_CTRL_TYPE;
    
    // CTOR
    PacketCtrl(E_CTRL_TYPE a_eCtrlType) {
        // Called for transmission
        m_eCtrlType = a_eCtrlType;
        m_bIsAlive = false;
        m_bFlowControl = false;
        m_bIsLockedBySelf = false;
        m_bIsLockedByOthers = false;
    }
    
    PacketCtrl(unsigned char a_Type) {
        // Called on reception
        // TODO: abort if a_Type != 0x10
    }
    
    void SetCtrlType(E_CTRL_TYPE a_eCtrlType) {
        m_eCtrlType = a_eCtrlType;
    }
     
    void SetIsAlive(bool a_bIsAlive) {
        m_bIsAlive = a_bIsAlive;
    }
    
    void SetFlowControl(bool a_bFlowControl) {
        m_bFlowControl = a_bFlowControl;
    }
     
    void SetIsLockedBySelf(bool a_bIsLockedBySelf) {
        m_bIsLockedBySelf = a_bIsLockedBySelf;
    }
    
    void SetIsLockedByOthers(bool a_bIsLockedByOthers) {
        m_bIsLockedByOthers = a_bIsLockedByOthers;
    }
    
private:
    // Serializer and deserializer
    const std::vector<unsigned char> Serialize() const {
        std::vector<unsigned char> l_Buffer;
        return std::move(l_Buffer);
    }
    
    size_t BytesNeeded() const {
        // TODO
        return 0;
    }
    
    bool BytesReceived(const unsigned char *a_ReadBuffer, size_t a_BytesRead) {
        // TODO
        return (true); // no error
    }
    
    // Members
    E_CTRL_TYPE m_eCtrlType;
    bool m_bIsAlive;
    bool m_bFlowControl;
    bool m_bIsLockedBySelf;
    bool m_bIsLockedByOthers;
};

#endif // PACKET_CTRL_H
