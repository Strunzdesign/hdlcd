/**
 * \file StreamFrame.h
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

#ifndef STREAM_FRAME_H
#define STREAM_FRAME_H

#include <vector>
#include <assert.h>

class StreamFrame {
public:
    enum { E_HEADER_LENGTH = 3 };
    enum { E_MAX_BODY_LENGTH = 512 };

    // CTORs
    StreamFrame(): m_BodyLength(0) {
        m_Direction = 0;
        m_Data.reserve(E_HEADER_LENGTH + E_MAX_BODY_LENGTH);
    }
    StreamFrame(const std::vector<unsigned char> a_Payload, unsigned char a_Direction) {
        // Checks
        assert(a_Payload.size() <= E_MAX_BODY_LENGTH);

        // Assemble stream header and payload
        m_BodyLength = a_Payload.size();
        m_Direction  = a_Direction;
        m_Data.emplace_back(m_Direction);
        m_Data.emplace_back((htons(m_BodyLength) >> 8) & 0x00FF);
        m_Data.emplace_back((htons(m_BodyLength) >> 0) & 0x00FF);
        m_Data.insert(m_Data.end(), a_Payload.begin(), a_Payload.end());
    }

    const unsigned char* data() const {
        return m_Data.data();
    }

    unsigned char* data() {
        return m_Data.data();
    }

    std::size_t length() const {
        return E_HEADER_LENGTH + m_BodyLength;
    }

    const unsigned char* body() const {
        return m_Data.data() + E_HEADER_LENGTH;
    }

    unsigned char* body() {
        return m_Data.data() + E_HEADER_LENGTH;
    }
    
    // Receive and decode frame
    bool DecodeHeader() {
        m_Direction = m_Data[0];
        m_BodyLength = *(reinterpret_cast<uint16_t*>(&m_Data[1]));
        if (m_BodyLength > E_MAX_BODY_LENGTH) {
            m_BodyLength = 0;
            return false;
        } // if

        return true;
    }
    
    std::size_t GetBodyLength() const {
        return m_BodyLength;
    }
    
    // Direction
    unsigned char GetDirection() const {
        return m_Direction;
    }

private:
    std::vector<unsigned char> m_Data;
    uint16_t m_BodyLength;
    unsigned char m_Direction;
};

#endif // STREAM_FRAME_H
