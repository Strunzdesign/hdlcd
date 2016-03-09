/**
 * \file FrameParser.h
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

#ifndef HDLC_FRAME_PARSER_H
#define HDLC_FRAME_PARSER_H

#include <vector>
#include "Frame.h"
class ProtocolState;

class FrameParser {
public:
    FrameParser(ProtocolState& a_ProtocolState);
    void Reset();
    void AddReceivedRawBytes(const unsigned char* a_Buffer, size_t a_Bytes);
    
private:
    // Interal helpers
    size_t AddChunk(const unsigned char* a_Buffer, size_t a_Bytes);
    bool RemoveEscapeCharacters();
    Frame DeserializeFrame(const std::vector<unsigned char> &a_UnescapedBuffer) const;
    
    // Members
    ProtocolState& m_ProtocolState;

    enum { max_length = 1024 };
    std::vector<unsigned char> m_Buffer;
    bool m_bStartTokenSeen;
};

#endif // HDLC_FRAME_PARSER_H
