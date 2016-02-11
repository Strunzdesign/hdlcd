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

#include <cstdio>
#include <cstdlib>
#include <cstring>

class StreamFrame {
public:
    enum { E_HEADER_LENGTH = 2 };
    enum { E_MAX_BODY_LENGTH = 512 };

    StreamFrame(): m_BodyLength(0) {
    }

    const unsigned char* data() const {
        return m_Data;
    }

    unsigned char* data() {
        return m_Data;
    }

    std::size_t length() const {
        return E_HEADER_LENGTH + m_BodyLength;
    }

    const unsigned char* body() const {
        return m_Data + E_HEADER_LENGTH;
    }

    unsigned char* body() {
        return m_Data + E_HEADER_LENGTH;
    }

    std::size_t body_length() const {
        return m_BodyLength;
    }

    void body_length(std::size_t a_BodyLength) {
        m_BodyLength = a_BodyLength;
        if (m_BodyLength > E_MAX_BODY_LENGTH)
        m_BodyLength = E_MAX_BODY_LENGTH;
    }

    bool decode_header() {
        m_BodyLength = *(reinterpret_cast<uint16_t*>(m_Data));
        if (m_BodyLength > E_MAX_BODY_LENGTH) {
            m_BodyLength = 0;
            return false;
        }

        return true;
    }

    void encode_header() {
        *(reinterpret_cast<uint16_t*>(m_Data)) = m_BodyLength;
    }

private:
    unsigned char m_Data[E_HEADER_LENGTH + E_MAX_BODY_LENGTH];
    uint16_t m_BodyLength;
};

#endif // STREAM_FRAME_H
