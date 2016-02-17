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
#include <memory>
#include "Frame.h"
class ProtocolState;

class FrameParser: public std::enable_shared_from_this<FrameParser> {
public:
    FrameParser(std::shared_ptr<ProtocolState> a_ProtocolState);
    void AddReceivedRawBytes(const char* a_Buffer, size_t a_Bytes);
    
private:
    // Interal helpers
    size_t AddChunk(const char* a_Buffer, size_t a_Bytes);
    bool RemoveEscapeCharacters();
    Frame DeserializeFrame(const std::vector<unsigned char> &a_UnescapedBuffer) const;
    
    /*
    void SendSABM() {
        // Trigger transmission of SABM
        m_FrameGenerator->SendSABM();
        m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
        auto self(shared_from_this());
        m_Timer.async_wait([this, self](const boost::system::error_code&) {
            if (m_HDLCType == HDLC_TYPE_UNKNOWN) {
                if (--m_NbrOfSABM) {
                    m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
                    SendSABM();
                } else {
                    std::cout << "SABM->UA timeout: we only have HDLC_TYPE_REDUCED" << std::endl;
                    m_HDLCType = HDLC_TYPE_REDUCED;
                } // else
            }
        });
    }
    */
    
    // Members
    std::shared_ptr<ProtocolState> m_ProtocolState;

    enum { max_length = 1024 };
    std::vector<unsigned char> m_Buffer;
    bool m_bStartTokenSeen;
};

#endif // HDLC_FRAME_PARSER_H
