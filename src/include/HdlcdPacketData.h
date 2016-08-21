/**
 * \file HdlcdPacketData.h
 * \brief 
 *
 * Copyright (c) 2016, Florian Evers, florian-evers@gmx.de
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 
 *     (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.  
 *     
 *     (3)The name of the author may not be used to
 *     endorse or promote products derived from this software without
 *     specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HDLCD_PACKET_DATA_H
#define HDLCD_PACKET_DATA_H

#include "HdlcdPacket.h"

class HdlcdPacketData: public HdlcdPacket {
public:
    static HdlcdPacketData CreatePacket(const std::vector<unsigned char> a_Payload, bool a_bReliable, bool a_bInvalid = false, bool a_bWasSent = false) {
        // Called for transmission
        HdlcdPacketData l_PacketData;
        l_PacketData.m_Payload = std::move(a_Payload);
        l_PacketData.m_bReliable = a_bReliable;
        l_PacketData.m_bInvalid = a_bInvalid;
        l_PacketData.m_bWasSent = a_bWasSent;
        return l_PacketData;
    }

    static std::shared_ptr<HdlcdPacketData> CreateDeserializedPacket(unsigned char a_Type) {
        // Called on reception: evaluate type field
        auto l_PacketData(std::shared_ptr<HdlcdPacketData>(new HdlcdPacketData));
        bool l_bReserved = (a_Type & 0x08); // TODO: abort if set
        l_PacketData->m_bReliable = (a_Type & 0x04);
        l_PacketData->m_bInvalid  = (a_Type & 0x02);
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
    bool GetInvalid()  const { return m_bInvalid; }
    bool GetWasSent()  const { return m_bWasSent; }
    
private:
    // Private CTOR
    HdlcdPacketData() {
        m_bReliable = false;
        m_bInvalid = false;
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
        if (m_bInvalid)  { l_Type |= 0x02; }
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
    bool m_bInvalid;
    bool m_bWasSent;
    typedef enum {
        DESERIALIZE_SIZE = 0,
        DESERIALIZE_DATA = 1,
        DESERIALIZE_FULL = 2
    } E_DESERIALIZE;
    E_DESERIALIZE m_eDeserialize;
    size_t m_BytesRemaining;
};

#endif // HDLCD_PACKET_DATA_H
