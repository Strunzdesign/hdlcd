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
#include "../SerialPortHandler.h"
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
    m_bPeerStoppedFlow = false;
    m_bPeerStoppedFlowSendData = false;
    m_bPeerRequiresAck = false;
    m_HDLCType = HDLC_TYPE_UNKNOWN;
    m_PortState = PORT_STATE_BAUDRATE_UNKNOWN;
    m_SREJs.clear();
    m_WaitQueueReliable.clear();
    m_WaitQueueUnreliable.clear();
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
    if (m_bStarted) {
        // Stop the state machine
        Reset();
    } // if
}

void ProtocolState::Shutdown() {
    Reset();
    m_SerialPortHandler.reset();
    m_FrameParser.reset();
}

void ProtocolState::SendPayload(const std::vector<unsigned char> &a_Payload, bool a_bReliable) {
    // Queue payload for later framing
    if (a_bReliable) {
        // TODO: assure that the size does not grow without limits!
        m_WaitQueueReliable.emplace_back(std::move(a_Payload));
    } else {
        // TODO: assure that the size does not grow without limits!
        m_WaitQueueUnreliable.emplace_back(std::move(a_Payload));
    } // else

    bool l_bSendReliableFrames = m_bStarted;
    l_bSendReliableFrames |= (m_bPeerStoppedFlow == false);
    l_bSendReliableFrames |= (m_WaitQueueReliable.empty() == false);
    bool l_bSendUnreliableFrames = m_bStarted;
    l_bSendUnreliableFrames |= (m_WaitQueueUnreliable.empty() == false);
    if ((m_bAwaitsNextHDLCFrame) && ((l_bSendReliableFrames) || (l_bSendUnreliableFrames))) {
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

void ProtocolState::AddReceivedRawBytes(const unsigned char* a_Buffer, size_t a_Bytes) {
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
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_RAW, a_Payload, false, a_bMessageValid, false); // not escaped
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_DISSECTED, a_Frame.Dissect(), false, a_bMessageValid, false);
    
    // Stop here if the frame was considered broken
    if (a_bMessageValid == false) {
        return;
    } // if
    
    if ((m_PortState == PORT_STATE_BAUDRATE_UNKNOWN) || (m_PortState == PORT_STATE_BAUDRATE_PROBE_SENT)) {
        // Found the correct baud rate
        m_PortState = PORT_STATE_BAUDRATE_FOUND;
        m_Timer.cancel();
        m_SerialPortHandler->PropagateSerialPortState();
    } // if
    
    // Go ahead interpreting the frame we received
    if (a_Frame.HasPayload()) {
        // I-Frame or U-Frame with UI
        m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_PAYLOAD, a_Frame.GetPayload(), a_Frame.IsIFrame(), true, false);
        
        // If it is an I-Frame, the data may have to be acked
        if (a_Frame.IsIFrame()) {
            if (m_RSeqIncoming != a_Frame.GetSSeq()) {
                m_SREJs.clear();
                for (unsigned char it = m_RSeqIncoming; (it & 0x07) != a_Frame.GetSSeq(); ++it) {
                    m_SREJs.emplace_back(it);
                } // for
            } // if

            // This does not respect gaps and retransmissions yet, which cannot be implemented using this reduced version of HDLC!
            m_bPeerRequiresAck = true;
            m_RSeqIncoming = ((a_Frame.GetSSeq() + 1) & 0x07);
        } // if
    } // if
    
    // Check the various types of ACKs and NACKs
    if ((a_Frame.IsIFrame()) || (a_Frame.IsSFrame())) {
        if ((a_Frame.IsIFrame()) || (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_RR)) {
            // Now we know the start of the window the receiver expects and which segments it allows us to send
            m_RSeqOutgoing = a_Frame.GetRSeq();
            if (m_bPeerStoppedFlow) {
                // The peer restarted the flow: RR clears RNR condition
                m_bPeerStoppedFlow = false;
                m_SSeqOutgoing = m_RSeqOutgoing;
                m_Timer.cancel();
                m_SerialPortHandler->PropagateSerialPortState();
            } // if
        } else if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_RNR) {
            // The peer wants us to stop sending subsequent data
            m_RSeqOutgoing = a_Frame.GetRSeq();
            m_bPeerStoppedFlow = true;
            m_SerialPortHandler->PropagateSerialPortState();
        } else if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_REJ) {
            // The peer requests for go-back-N. We have to retransmit all affected packets
            if (m_bPeerStoppedFlow) {
                // The peer restarted the flow: REJ clears RNR condition
                m_bPeerStoppedFlow = false;
                m_SSeqOutgoing = m_RSeqOutgoing;
                m_Timer.cancel();
                m_SerialPortHandler->PropagateSerialPortState();
            } // if
        } else {
            assert(a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_SREJ);
            // The peer requests for the retransmission of a single segment with a specific sequence number
            // This cannot be implemented using this reduced version of HDLC!
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
        auto self(shared_from_this());
        m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
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
        if ((m_WaitQueueReliable.empty()) && (m_WaitQueueUnreliable.empty()) && (!m_bPeerRequiresAck) && (m_SREJs.empty())) {
            // Nothing to transmit
            return;
        } // if

        // Send SREJ?
        if (m_SREJs.empty() == false) {
            // Send SREJs first
            l_Frame = PrepareSFrameSREJ();
        } // if
        
        // Send I-frame with data?
        if (l_Frame.IsEmpty() && ((m_WaitQueueReliable.empty() == false) && ((m_bPeerStoppedFlow == false) || m_bPeerStoppedFlowSendData))) {
            l_Frame = PrepareIFrame();
            if (m_bPeerStoppedFlowSendData) {
                m_bPeerStoppedFlowSendData = false;
                auto self(shared_from_this());
                m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
                m_Timer.async_wait([this, self](const boost::system::error_code& ec) {
                    if (!ec) {
                        if (m_bPeerStoppedFlow) {
                            m_bPeerStoppedFlowSendData = true;
                        } // if

                        OpportunityForTransmission();
                    } // if
                });
            } else {
                m_WaitQueueReliable.pop_front();
                m_SSeqOutgoing = ((m_SSeqOutgoing + 1) & 0x07);
            } // else
            
            m_bPeerRequiresAck = false;
        } // if
        
        // Send RR?
        if (l_Frame.IsEmpty() && (m_bPeerRequiresAck)) {
            l_Frame = PrepareSFrameRR();
            m_bPeerRequiresAck = false;
        } // if
        
        // Send U-UI-frame with data?
        if (l_Frame.IsEmpty() && (m_WaitQueueUnreliable.empty() == false)) {
            l_Frame = PrepareUFrameUI();
            m_WaitQueueUnreliable.pop_front();
        } // if
    } // else

    if (l_Frame.IsEmpty() == false) {
        // Deliver unescaped frame to clients that have interest
        m_bAwaitsNextHDLCFrame = false;
        auto l_HDLCFrameBuffer = FrameGenerator::SerializeFrame(l_Frame);
        m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_RAW, l_HDLCFrameBuffer, l_Frame.IsIFrame(), true, true); // not escaped
        m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_DISSECTED, l_Frame.Dissect(), l_Frame.IsIFrame(), true, true);
        m_SerialPortHandler->DeliverHDLCFrame(std::move(FrameGenerator::EscapeFrame(l_HDLCFrameBuffer)));
    } // if
}

Frame ProtocolState::PrepareIFrame() {
    // Fresh Payload to be sent is available.
    assert(m_WaitQueueReliable.empty() == false);
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_PAYLOAD, m_WaitQueueReliable.front(), true, true, true);

    // Prepare I-Frame    
    Frame l_Frame;
    l_Frame.SetAddress(0x30);
    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_I);
    l_Frame.SetPF(false);
    l_Frame.SetSSeq(m_SSeqOutgoing);
    l_Frame.SetRSeq(m_RSeqIncoming);
    l_Frame.SetPayload(m_WaitQueueReliable.front());
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

Frame ProtocolState::PrepareUFrameUI() {
    assert(m_WaitQueueUnreliable.empty() == false);
    m_SerialPortHandler->DeliverBufferToClients(HDLCBUFFER_PAYLOAD, m_WaitQueueUnreliable.front(), false, true, true);

    // Prepare UI-Frame
    Frame l_Frame;
    l_Frame.SetAddress(0x30);
    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_UI);
    l_Frame.SetPF(false);
    l_Frame.SetPayload(m_WaitQueueUnreliable.front());
    return(std::move(l_Frame));
}

Frame ProtocolState::PrepareUFrameTEST() {
    Frame l_Frame;
    l_Frame.SetAddress(0x30);
    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_TEST);
    l_Frame.SetPF(false);
    return(std::move(l_Frame));
}
