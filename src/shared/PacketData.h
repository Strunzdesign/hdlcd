/**
 * \file PacketData.h
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

#ifndef PACKET_DATA_H
#define PACKET_DATA_H

#include "Packet.h"

class PacketData: public Packet {
public:
    // CTOR
    PacketData(const std::vector<unsigned char> a_Payload, bool a_bReliable, bool a_bValid, bool a_bWasSent):
        // Called for transmission
        m_Payload(std::move(a_Payload)),
        m_bReliable(a_bReliable),
        m_bValid(a_bValid),
        m_bWasSent(a_bWasSent) {
        m_eDeserialize = DESERIALIZE_FULL;
        m_BytesRemaining = 0;
    }
    
    static std::shared_ptr<PacketData> CreateDeserializedPacket(unsigned char a_Type) {
        // Called on reception: evaluate type field
        auto l_PacketData(std::shared_ptr<PacketData>(new PacketData));
        bool l_bReserved = (a_Type & 0x08); // TODO: abort if set
        l_PacketData->m_bReliable = (a_Type & 0x04);
        l_PacketData->m_bValid    = (a_Type & 0x02);
        l_PacketData->m_bWasSent  = (a_Type & 0x01);
        l_PacketData->m_eDeserialize = DESERIALIZE_SIZE;
        l_PacketData->m_BytesRemaining = 2;
        return l_PacketData;
    }
    
    const std::vector<unsigned char>& GetData() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        return m_Payload;
    }
    
    bool GetReliable() const { return m_bReliable; }
    bool GetValid() const { return m_bValid; }
    bool GetWasSent() const { return m_bWasSent; }
    
private:
    // Private CTOR
    PacketData() {
        m_bReliable = false;
        m_bValid = false;
        m_bWasSent = false;
        m_eDeserialize = DESERIALIZE_FULL;
        m_BytesRemaining = 0;
    }

    // Serializer and deserializer
    const std::vector<unsigned char> Serialize() const {
        assert(m_eDeserialize == DESERIALIZE_FULL);
        assert(m_BytesRemaining == 0);
        std::vector<unsigned char> l_Buffer;
        
        // Prepare type field
        unsigned char l_Type = 0x00;
        if (m_bReliable) { l_Type |= 0x04; }
        if (m_bValid)    { l_Type |= 0x02; }
        if (m_bWasSent)  { l_Type |= 0x01; }
        l_Buffer.emplace_back(l_Type);
        
        // Prepare length field
        l_Buffer.emplace_back((htons(m_Payload.size()) >> 0) & 0x00FF);
        l_Buffer.emplace_back((htons(m_Payload.size()) >> 8) & 0x00FF);
        
        // Add payload
        l_Buffer.insert(l_Buffer.end(), m_Payload.begin(), m_Payload.end());
        return std::move(l_Buffer);
    }
    
    size_t BytesNeeded() const {
        return m_BytesRemaining;
    }
    
    bool BytesReceived(const unsigned char *a_ReadBuffer, size_t a_BytesRead) {
        switch (m_eDeserialize) {
        case DESERIALIZE_SIZE: {
            // Read length field
            assert((m_BytesRemaining > 0) && (m_BytesRemaining <= 2));
            assert(a_BytesRead);
            assert(a_BytesRead == 2); // TODO: dirty, might break!
            assert(m_Payload.empty());
            m_BytesRemaining = ntohs(*(reinterpret_cast<const uint16_t*>(a_ReadBuffer)));
            m_eDeserialize = DESERIALIZE_DATA;
            break;
        }
        case DESERIALIZE_DATA: {
            // Read payload
            assert(m_BytesRemaining <= a_BytesRead);
            assert(a_BytesRead);
            m_Payload.insert(m_Payload.end(), a_ReadBuffer, (a_ReadBuffer + a_BytesRead));
            m_BytesRemaining -= a_BytesRead;
            if (m_BytesRemaining == 0) {
                m_eDeserialize = DESERIALIZE_FULL;
            } // if

            break;
        }
        case DESERIALIZE_FULL:
        default:
            assert(false);
        } // switch
        
        return (true); // no error
    }
    
    // Members
    std::vector<unsigned char> m_Payload;
    bool m_bReliable;
    bool m_bValid;
    bool m_bWasSent;
    typedef enum {
        DESERIALIZE_SIZE = 0,
        DESERIALIZE_DATA = 1,
        DESERIALIZE_FULL = 2
    } E_DESERIALIZE;
    E_DESERIALIZE m_eDeserialize;
    size_t m_BytesRemaining;
};

#endif // PACKET_DATA_H
