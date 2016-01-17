/**
 * \file ProtocolState.cpp
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

#include "ProtocolState.h"
#include "../ComPortHandler.h"
#include "FrameParser.h"
#include "FrameGenerator.h"

ProtocolState::ProtocolState(std::shared_ptr<ComPortHandler> a_ComPortHandler, boost::asio::io_service& a_IOService) {
    m_bSendQueueEmpty = true;
    m_SSEQ = 0;
    m_RSEQ = 0;
    m_HDLCType = HDLC_TYPE_UNKNOWN;
    m_ComPortHandler = a_ComPortHandler;
}

void ProtocolState::Start() {
    m_FrameParser = std::make_shared<FrameParser>(shared_from_this());
}

void ProtocolState::Stop() {
    m_ComPortHandler.reset();
    m_FrameParser.reset();
}

void ProtocolState::SendPayload(const std::vector<unsigned char> &a_Payload) {
    // Fresh Payload to be sent is available. Queue it for later framing
    if (m_bSendQueueEmpty) {
        m_bSendQueueEmpty = false;
        Frame l_Frame;
        l_Frame.SetAddress(0x30);
        l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_I);
        l_Frame.SetPF(false);
        l_Frame.SetSSeq(m_SSEQ);
        l_Frame.SetRSeq(m_RSEQ);
        l_Frame.SetPayload(std::move(a_Payload));

        std::cout << "Sending " << l_Frame.GetReadableDescription() << std::endl;
        m_ComPortHandler->DeliverHDLCFrame(FrameGenerator::SerializeFrame(l_Frame));
        
        // Increase SSeq
        m_SSEQ = ((m_SSEQ + 1) & 0x07);
    } // if
    
}

void ProtocolState::SendQueueIsEmpty() {
    // The SendQueue is empty. If a fresh frame can be sent, create and deliver it now
    m_bSendQueueEmpty = true;
}

void ProtocolState::AddReceivedRawBytes(const char* a_Buffer, size_t a_Bytes) {
    m_FrameParser->AddReceivedRawBytes(a_Buffer, a_Bytes);
}

void ProtocolState::DeliverDeserializedFrame(const Frame& a_Frame) {
    std::cout << "Received " << a_Frame.GetReadableDescription() << std::endl;
    if (a_Frame.HasPayload()) {
        m_ComPortHandler->DeliverPayloadToClients(a_Frame.GetPayload());
    } // if
}
