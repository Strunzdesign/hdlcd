/**
 * \file ProtocolState.h
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

#ifndef HDLC_PROTOCOL_STATE_H
#define HDLC_PROTOCOL_STATE_H

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <boost/asio.hpp>
#include <string.h>
#include "Frame.h"
class ComPortHandler;
class FrameParser;

class ProtocolState: public std::enable_shared_from_this<ProtocolState> {
public:
    ProtocolState(std::shared_ptr<ComPortHandler> a_ComPortHandler, boost::asio::io_service& a_IOService);
    void Start();
    void Stop();

    void SendPayload(const std::vector<unsigned char> &a_Payload);
    void SendQueueIsEmpty();
    void AddReceivedRawBytes(const char* a_Buffer, size_t a_Bytes);
    void InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame, bool a_bMessageValid);

private:
    // Members
    bool m_bSendQueueEmpty;
    unsigned char m_SSeqOutgoing; // The sequence number we are going to use for the transmission of the next packet
    unsigned char m_RSeqOutgoing; // The start of the RX window offered by our peer, defines which packets it expects
    unsigned char m_SSeqIncoming; // The sequence number we expect our peer to use for the next packet directed to us
    unsigned char m_RSeqIncoming; // The start of the RX window we offer our peer, defines which packets we expect
    
    // Parser and generator
    std::shared_ptr<ComPortHandler> m_ComPortHandler;
    std::shared_ptr<FrameParser>    m_FrameParser;
    
    // Determine type of HDLC peer
    typedef enum {
        HDLC_TYPE_UNKNOWN = 0,
        HDLC_TYPE_REDUCED = 1,
        HDLC_TYPE_FULL    = 2,
    } T_HDLC_TYPE;
    T_HDLC_TYPE m_HDLCType;
};

#endif // HDLC_PROTOCOL_STATE_H
