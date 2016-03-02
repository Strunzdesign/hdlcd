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

#include <memory>
#include "Packet.h"

class PacketCtrl: public Packet {
public:
    // Types of control packets
    typedef enum {
        CTRL_TYPE_PORT_STATUS = 0x00,
        CTRL_TYPE_ECHO        = 0x10,
        CTRL_TYPE_KEEP_ALIVE  = 0x20,
        CTRL_TYPE_PORT_KILL   = 0x30,
        CTRL_TYPE_UNSET       = 0xFF
    } E_CTRL_TYPE;

    static std::shared_ptr<PacketCtrl> CreateDeserializedPacket(unsigned char a_Type) {
        // Called on reception: evaluate type field
        auto l_PacketCtrl(std::shared_ptr<PacketCtrl>(new PacketCtrl));
        // TODO: abort if type is not 0x10
        l_PacketCtrl->m_eDeserialize = DESERIALIZE_CTRL;
        l_PacketCtrl->m_BytesRemaining = 1;
        return l_PacketCtrl;
    }
    
    //
    static PacketCtrl CreatePortStatusRequest(bool a_bLockSerialPort) {
        PacketCtrl l_PacketCtrl;
        l_PacketCtrl.m_eCtrlType = CTRL_TYPE_PORT_STATUS;
        l_PacketCtrl.m_bLockSerialPort = a_bLockSerialPort;
        return l_PacketCtrl;
    }
    
    static PacketCtrl CreatePortStatusResponse(bool a_bIsAlive, bool a_bFlowSuspended, bool a_bIsLockedByOthers, bool a_bIsLockedBySelf) {
        PacketCtrl l_PacketCtrl;
        l_PacketCtrl.m_eCtrlType = CTRL_TYPE_PORT_STATUS;
        l_PacketCtrl.m_bAlive = a_bIsAlive;
        l_PacketCtrl.m_bFlowSuspended = a_bFlowSuspended;
        l_PacketCtrl.m_bLockedByOthers = a_bIsLockedByOthers;
        l_PacketCtrl.m_bLockedBySelf = a_bIsLockedBySelf;
        return l_PacketCtrl;
    }
    
    static PacketCtrl CreateEchoRequest() {
        PacketCtrl l_PacketCtrl;
        l_PacketCtrl.m_eCtrlType = CTRL_TYPE_ECHO;
        return l_PacketCtrl;
    }
    
    static PacketCtrl CreateKeepAliveRequest() {
        PacketCtrl l_PacketCtrl;
        l_PacketCtrl.m_eCtrlType = CTRL_TYPE_KEEP_ALIVE;
        return l_PacketCtrl;
    }
    
    static PacketCtrl CreatePortKillRequest() {
        PacketCtrl l_PacketCtrl;
        l_PacketCtrl.m_eCtrlType = CTRL_TYPE_PORT_KILL;        
        return l_PacketCtrl;
    }
    
    // Getters
    E_CTRL_TYPE GetPacketType() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        return m_eCtrlType;
    }

    bool GetIsAlive() const {
        assert(m_eCtrlType == CTRL_TYPE_PORT_STATUS);
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        return m_bAlive;
    }
    
    bool GetIsFlowSuspended() const {
        assert(m_eCtrlType == CTRL_TYPE_PORT_STATUS);
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        return m_bFlowSuspended;
    }

    bool GetIsLockedByOthers() const {
        assert(m_eCtrlType == CTRL_TYPE_PORT_STATUS);
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        return m_bLockedByOthers;
    }

    bool GetIsLockedBySelf() const {
        assert(m_eCtrlType == CTRL_TYPE_PORT_STATUS);
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        return m_bLockedBySelf;
    }

    bool GetDesiredLockState() const {
        assert(m_eCtrlType == CTRL_TYPE_PORT_STATUS);
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        return m_bLockSerialPort;
    }

private:
    // Private CTOR
    PacketCtrl() {
        m_bAlive = false;
        m_bFlowSuspended = false;
        m_bLockedByOthers = false;
        m_bLockedBySelf = false;
        m_bLockSerialPort = false;
        m_eCtrlType = CTRL_TYPE_UNSET;
        m_eDeserialize = DESERIALIZE_FULL;
        m_BytesRemaining = 0;
    }

    // Serializer and deserializer
    const std::vector<unsigned char> Serialize() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        std::vector<unsigned char> l_Buffer;
        l_Buffer.emplace_back(0x10);
        
        // Prepare control field
        unsigned char l_Control = 0x00;
        switch (m_eCtrlType) {
            case CTRL_TYPE_PORT_STATUS:
                l_Control = 0x00;
                if (m_bAlive)          { l_Control |= 0x08; }
                if (m_bFlowSuspended)  { l_Control |= 0x04; }
                if (m_bLockedByOthers) { l_Control |= 0x02; }
                if (m_bLockedBySelf)   { l_Control |= 0x01; }
                if (m_bLockSerialPort) { l_Control |= 0x01; } // for requests
                break;
            case CTRL_TYPE_ECHO:
                l_Control = 0x10;
                break;
            case CTRL_TYPE_KEEP_ALIVE:
                l_Control = 0x20;
                break;
            case CTRL_TYPE_PORT_KILL:
                l_Control = 0x30;
                break;
            default:
                assert(false);
        } // switch

        l_Buffer.emplace_back(l_Control);
        return std::move(l_Buffer);
    }
    
    size_t BytesNeeded() const {
        return m_BytesRemaining;
    }
    
    bool BytesReceived(const unsigned char *a_ReadBuffer, size_t a_BytesRead) {
        switch (m_eDeserialize) {
        case DESERIALIZE_CTRL: {
            // Read control byte
            assert(m_BytesRemaining == 1);
            assert(a_BytesRead == 1);
            
            // Parse control byte
            const unsigned char &l_Control = a_ReadBuffer[0];
            switch (l_Control & 0xF0) {
            case 0x00: {
                m_eCtrlType = CTRL_TYPE_PORT_STATUS;
                // For both requests and responses
                // TODO: check other bits!
                m_bAlive          = (l_Control & 0x08);
                m_bFlowSuspended  = (l_Control & 0x04);
                m_bLockedByOthers = (l_Control & 0x02);
                m_bLockedBySelf   = (l_Control & 0x01);
                m_bLockSerialPort = (l_Control & 0x01); // for requests
                break;
            }
            case 0x10: {
                m_eCtrlType = CTRL_TYPE_ECHO;
                break;
            }
            case 0x20: {
                m_eCtrlType = CTRL_TYPE_KEEP_ALIVE;
                break;
            }
            case 0x30: {
                m_eCtrlType = CTRL_TYPE_PORT_KILL;
                break;
            }
            default:
                // Unknown. TODO: check
                m_eCtrlType = CTRL_TYPE_UNSET;
            } // switch
            
            m_BytesRemaining = 0;
            m_eDeserialize = DESERIALIZE_FULL;
            break;
        }
        case DESERIALIZE_FULL:
        default:
            assert(false);
        } // switch
        
        return (true); // no error
    }

    // Members
    bool m_bAlive;
    bool m_bFlowSuspended;
    bool m_bLockedByOthers;
    bool m_bLockedBySelf;
    bool m_bLockSerialPort;
    
    E_CTRL_TYPE m_eCtrlType;
    typedef enum {
        DESERIALIZE_CTRL = 0,
        DESERIALIZE_FULL = 1
    } E_DESERIALIZE;
    E_DESERIALIZE m_eDeserialize;
    size_t m_BytesRemaining;
};

#endif // PACKET_CTRL_H
