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
#include <assert.h>
#include "../SerialPort/SerialPortHandler.h"
#include "../../shared/Direction.h"
#include "FrameParser.h"
#include "FrameGenerator.h"

ProtocolState::ProtocolState(std::shared_ptr<SerialPortHandler> a_SerialPortHandler, boost::asio::io_service& a_IOService): m_Timer(a_IOService) {
    m_SerialPortHandler = a_SerialPortHandler;
    Reset();
}

void ProtocolState::Reset() {
    m_Timer.cancel();
    m_bStarted = false;
    m_bAwaitsNextHDLCFrame = true;
    m_SSeqOutgoing = 0;
    m_RSeqOutgoing = 0;
    m_SSeqIncoming = 0;
    m_RSeqIncoming = 0;
    m_bPeerRequiresAck = false;
    m_HDLCType = HDLC_TYPE_UNKNOWN;
    m_PortState = PORT_STATE_BAUDRATE_UNKNOWN;
    m_SREJs.clear();
    m_PayloadWaitQueue.clear();
    if (m_FrameParser) {
        m_FrameParser->Reset();
    } // if
}

void ProtocolState::Start() {
    if (!m_FrameParser) {
        m_FrameParser = std::make_shared<FrameParser>(shared_from_this());
    } // if

    // Start the state machine
    Reset();
    m_bStarted = true;
    OpportunityForTransmission(); // trigger baud rate detection
}

void ProtocolState::Stop() {
    // Stop the state machine
    Reset();
}

void ProtocolState::Shutdown() {
    Reset();
    m_SerialPortHandler.reset();
    m_FrameParser.reset();
}

void ProtocolState::SendPayload(const std::vector<unsigned char> &a_Payload) {
    // Checks
    if (!m_bStarted) {
        return;
    } // if

    // Fresh Payload to be sent is available.
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_PAYLOAD, DIRECTION_SENT, a_Payload, true);
    
    // Queue payload for later framing
    m_PayloadWaitQueue.emplace_back(std::move(a_Payload));
    if (m_bAwaitsNextHDLCFrame) {
        OpportunityForTransmission();
    } // if
}

void ProtocolState::TriggerNextHDLCFrame() {
    // Checks
    if (!m_bStarted) {
        return;
    } // if

    // The SerialPortHandler is ready to transmit the next HDLC frame
    m_bAwaitsNextHDLCFrame = true;
    OpportunityForTransmission();
}

void ProtocolState::AddReceivedRawBytes(const char* a_Buffer, size_t a_Bytes) {
    // Checks
    if (!m_bStarted) {
        return;
    } // if

    m_FrameParser->AddReceivedRawBytes(a_Buffer, a_Bytes);
}

void ProtocolState::InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame, bool a_bMessageValid) {
    // Checks
    if (!m_bStarted) {
        return;
    } // if

    // Deliver raw frame to clients that have interest
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_RAW, DIRECTION_RCVD, a_Payload, a_bMessageValid); // not escaped
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_DISSECTED, DIRECTION_RCVD, a_Frame.Dissect(), a_bMessageValid);
    
    // Stop here if the frame was considered broken
    if (a_bMessageValid == false) {
        return;
    } // if
    
    if ((m_PortState == PORT_STATE_BAUDRATE_UNKNOWN) || (m_PortState == PORT_STATE_BAUDRATE_PROBE_SENT)) {
        // Ongoing baud rate detection
        if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_U_TEST) {
            // Found the correct baud rate
            m_PortState = PORT_STATE_BAUDRATE_FOUND;
            m_Timer.cancel();
        } else {
            // Nothing to do until the correct baud rate was determined
            return;
        } // else
    } // if
    
    // Go ahead interpreting the frame we received
    if (a_Frame.HasPayload()) {
        // I-Frame or U-Frame with UI
        m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_PAYLOAD, DIRECTION_RCVD, a_Frame.GetPayload(), true);
        
        // If it is an I-Frame, the data may have to be acked
        if (a_Frame.IsIFrame()) {
            if (m_RSeqIncoming != a_Frame.GetSSeq()) {
                m_SREJs.clear();
                for (unsigned char it = m_RSeqIncoming; (it & 0x07) != a_Frame.GetSSeq(); ++it) {
                    m_SREJs.emplace_back(it);
                } // for
            } // if

            // TODO: does not respect gaps and retransmissions yet
            m_bPeerRequiresAck = true;
            m_RSeqIncoming = ((a_Frame.GetSSeq() + 1) & 0x07);
        } // if
    } // if
    
    // Check the various types of ACKs and NACKs
    if ((a_Frame.IsIFrame()) || (a_Frame.IsSFrame())) {
        if ((a_Frame.IsIFrame()) || (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_RR)) {
            // Now we know the start of the window the receiver expects and which segments it allows us to send
            m_RSeqOutgoing = a_Frame.GetRSeq();
        } else if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_RNR) {
            // The peer wants us to stop sending subsequent data
            // TODO: not implemented yet
        } else if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_REJ) {
            // The peer requests for go-back-N. We have to retransmit all affected packets
            // TODO: not implemented yet
        } else {
            assert(a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_SREJ);
            // The peer requests for the retransmission of a single segment with a specific sequence number
            // TODO: not implemented yet
        } // else
    } // if
    
    if (m_bAwaitsNextHDLCFrame) {
        // Check if we have to send something now
        OpportunityForTransmission();
    } // if
}

void ProtocolState::OpportunityForTransmission() {
    // Checks
    assert(m_bAwaitsNextHDLCFrame);
    
    Frame l_Frame;
    if (m_PortState == PORT_STATE_BAUDRATE_UNKNOWN) {
        // The correct baud rate setting is unknown yet. Send an U-TEST frame.
        m_PortState = PORT_STATE_BAUDRATE_PROBE_SENT;
        l_Frame = PrepareUFrameTEST();
        
        // Setup retransmission timer
        m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
        auto self(shared_from_this());
        m_Timer.async_wait([this, self](const boost::system::error_code& ec) {
            if (!ec) {
                m_PortState = PORT_STATE_BAUDRATE_UNKNOWN;
                m_SerialPortHandler->ChangeBaudRate();
                OpportunityForTransmission();
            } // if
        });
    } else if (m_PortState == PORT_STATE_BAUDRATE_PROBE_SENT) {
        // We have to wait until the timer expires
        return;
    } else {
        assert(m_PortState == PORT_STATE_BAUDRATE_FOUND);
        if ((m_PayloadWaitQueue.empty()) && (!m_bPeerRequiresAck) && (m_SREJs.empty())) {
            return;
        } // if

        if (m_SREJs.empty() == false) {
            // Send SREJs first
            l_Frame = PrepareSFrameSREJ();
        } else if (m_PayloadWaitQueue.empty() == false) {
            l_Frame = PrepareIFrame();
            m_bPeerRequiresAck = false;
            m_SSeqOutgoing = ((m_SSeqOutgoing + 1) & 0x07);
        } else {
            l_Frame = PrepareSFrameRR();
            m_bPeerRequiresAck = false;
        } // else
    } // else if
    
    // Deliver unescaped frame to clients that have interest
    m_bAwaitsNextHDLCFrame = false;
    auto l_HDLCFrameBuffer = FrameGenerator::SerializeFrame(l_Frame);
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_RAW, DIRECTION_SENT, l_HDLCFrameBuffer, true); // not escaped
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_DISSECTED, DIRECTION_SENT, l_Frame.Dissect(), true);
    m_SerialPortHandler->DeliverHDLCFrame(std::move(FrameGenerator::EscapeFrame(l_HDLCFrameBuffer)));
}

Frame ProtocolState::PrepareIFrame() {
    Frame l_Frame;
    l_Frame.SetAddress(0x30);
    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_I);
    l_Frame.SetPF(false);
    l_Frame.SetSSeq(m_SSeqOutgoing);
    l_Frame.SetRSeq(m_RSeqIncoming);
    l_Frame.SetPayload(std::move(m_PayloadWaitQueue.front()));
    m_PayloadWaitQueue.pop_front();
    return(std::move(l_Frame));
}

Frame ProtocolState::PrepareSFrameRR() {
    Frame l_Frame;
    l_Frame.SetAddress(0x30);
    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_S_RR);
    l_Frame.SetPF(false);
    l_Frame.SetRSeq(m_RSeqIncoming);
    return(std::move(l_Frame));
}

Frame ProtocolState::PrepareSFrameSREJ() {
    Frame l_Frame;
    l_Frame.SetAddress(0x30);
    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_S_SREJ);
    l_Frame.SetPF(false);
    l_Frame.SetRSeq(m_SREJs.front());
    m_SREJs.pop_front();
    return(std::move(l_Frame));
}

Frame ProtocolState::PrepareUFrameTEST() {
    Frame l_Frame;
    l_Frame.SetAddress(0x30);
    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_TEST);
    l_Frame.SetPF(true); // Hotfix
    return(std::move(l_Frame));
}
