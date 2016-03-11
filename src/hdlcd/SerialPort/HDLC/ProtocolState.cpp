/**
 * \file ProtocolState.cpp
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

#include "ProtocolState.h"
#include <assert.h>
#include "../SerialPortHandler.h"
#include "FrameGenerator.h"

ProtocolState::ProtocolState(std::shared_ptr<SerialPortHandler> a_SerialPortHandler, boost::asio::io_service& a_IOService): m_SerialPortHandler(a_SerialPortHandler), m_Timer(a_IOService), m_FrameParser(*this) {
    // Initialize alive state helper
    m_AliveState = std::make_shared<AliveState>(a_IOService);
    m_AliveState->SetSendProbeCallback([this]() {
        m_bSendProbe = true;
        OpportunityForTransmission();
    });
    m_AliveState->SetChangeBaudrateCallback([this]() {
        m_SerialPortHandler->ChangeBaudRate();
        m_SerialPortHandler->PropagateSerialPortState();
    });
    
    Reset();
}

void ProtocolState::Reset() {
    m_AliveState->Stop();
    m_Timer.cancel();
    m_bStarted = false;
    m_bAwaitsNextHDLCFrame = true;
    m_SSeqOutgoing = 0;
    m_SSeqIncoming = 0;
    m_RSeqIncoming = 0;
    m_bSendProbe = false;
    m_bPeerStoppedFlow = false;
    m_bPeerRequiresAck = false;
    m_bWaitForAck = false;
    m_SREJs.clear();
    m_FrameParser.Reset();
}

void ProtocolState::Start() {
    // Start the state machine
    Reset();
    m_bStarted = true;
    m_AliveState->Start(); // trigger baud rate detection
    OpportunityForTransmission();
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

    m_FrameParser.AddReceivedRawBytes(a_Buffer, a_Bytes);
}

void ProtocolState::InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame, bool a_bMessageValid) {
    // Checks
    if (!m_bStarted) {
        return;
    } // if

    // Deliver raw frame to clients that have interest
    m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_RAW, a_Payload, false, a_bMessageValid, false); // not escaped
    m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_DISSECTED, a_Frame.Dissect(), false, a_bMessageValid, false);
    
    // Stop here if the frame was considered broken
    if (a_bMessageValid == false) {
        return;
    } // if
    
    // A valid frame was received
    if (m_AliveState->OnFrameReceived()) {
        m_SerialPortHandler->PropagateSerialPortState();
    } // if
    
    // Go ahead interpreting the frame we received
    if (a_Frame.HasPayload()) {
        // I-Frame or U-Frame with UI
        m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_PAYLOAD, a_Frame.GetPayload(), a_Frame.IsIFrame(), true, false);
        
        // If it is an I-Frame, the data may have to be acked
        if (a_Frame.IsIFrame()) {
            if (m_RSeqIncoming != a_Frame.GetSSeq()) {
                m_SREJs.clear();
                for (unsigned char it = m_RSeqIncoming; (it & 0x07) != a_Frame.GetSSeq(); ++it) {
                    m_SREJs.emplace_back(it);
                } // for
            } // if

            m_RSeqIncoming = ((a_Frame.GetSSeq() + 1) & 0x07);
            m_bPeerRequiresAck = true;
            
            // Currently, we send only one I-frame at a time and wait until the respective ACK is received
            if ((m_bWaitForAck) && (a_Frame.GetRSeq() == ((m_SSeqOutgoing + 1) & 0x07))) {
                // We found the respective sequence number to the last transmitted I-frame
                m_bWaitForAck = false;
                m_SSeqOutgoing = a_Frame.GetRSeq();
                m_Timer.cancel();
                m_WaitQueueReliable.pop_front();
            } // if
        } // if
    } // if
    
    // Check the various types of ACKs and NACKs
    if ((a_Frame.IsIFrame()) || (a_Frame.IsSFrame())) {
        if ((a_Frame.IsIFrame()) || (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_RR)) {
            if (m_bPeerStoppedFlow) {
                // The peer restarted the flow: RR clears RNR condition
                m_bPeerStoppedFlow = false;
                m_Timer.cancel();
            } // if

            // Currently, we send only one I-frame at a time and wait until the respective ACK is received
            if ((m_bWaitForAck) && (a_Frame.GetRSeq() == ((m_SSeqOutgoing + 1) & 0x07))) {
                // We found the respective sequence number to the last transmitted I-frame
                m_bWaitForAck = false;
                m_Timer.cancel();
                m_WaitQueueReliable.pop_front();
            } // if
            
            m_SSeqOutgoing = a_Frame.GetRSeq();
        } else if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_RNR) {
            // The peer wants us to stop sending subsequent data
            m_bPeerStoppedFlow = true;
            if ((m_bWaitForAck) && (a_Frame.GetRSeq() == ((m_SSeqOutgoing + 1) & 0x07))) {
                // We found the respective sequence number to the last transmitted I-frame
                m_bWaitForAck = false;
                m_Timer.cancel();
                m_WaitQueueReliable.pop_front();
            } // if
            
            m_SSeqOutgoing = a_Frame.GetRSeq();
        } else if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_REJ) {
            if (m_bPeerStoppedFlow) {
                // The peer restarted the flow: REJ clears RNR condition
                m_bPeerStoppedFlow = false;
                m_Timer.cancel();
            } // if
            
            // The peer requests for go-back-N. We have to retransmit all affected packets, but not with this version of HDLC.
            if ((m_bWaitForAck) && (a_Frame.GetRSeq() == m_SSeqOutgoing)) {
                // We found the respective sequence number to the last transmitted I-frame
                m_bWaitForAck = false;
                m_Timer.cancel();
            } // if
            
            m_SSeqOutgoing = ((a_Frame.GetRSeq() + 0x07) & 0x07);
        } else {
            assert(a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_S_SREJ);
            if (m_bPeerStoppedFlow) {
                // The peer restarted the flow: SREJ clears RNR condition
                m_bPeerStoppedFlow = false;
                m_Timer.cancel();
            } // if
            
            // The peer requests for the retransmission of a single segment with a specific sequence number
            // This cannot be implemented using this reduced version of HDLC!
            if ((m_bWaitForAck) && (a_Frame.GetRSeq() == m_SSeqOutgoing)) {
                // We found the respective sequence number to the last transmitted I-frame.
                // In this version of HDLC, this should not happen!
                m_bPeerStoppedFlow = false;
                m_Timer.cancel();
            } // if
        } // else
    } // if
    
    if (m_bAwaitsNextHDLCFrame) {
        // Check if we have to send something now
        OpportunityForTransmission();
    } // if
}

void ProtocolState::OpportunityForTransmission() {
    // Checks
    if (!m_bAwaitsNextHDLCFrame) {
        return;
    } // if
    
    Frame l_Frame;
    if (m_bSendProbe) {
        // The correct baud rate setting is unknown yet, or it has to be checked again. Send an U-TEST frame.
        m_bSendProbe = false;
        l_Frame = PrepareUFrameTEST();
    } // if
    
    if (l_Frame.IsEmpty() && m_AliveState->IsAlive()) {
        // Check the state of the wait queues
        if ((m_WaitQueueReliable.empty()) && (m_WaitQueueUnreliable.empty()) && (!m_bPeerRequiresAck) && (m_SREJs.empty())) {
            // Nothing to transmit. Query all clients for data.
            m_SerialPortHandler->QueryForPayload();
            
            // If we got data, the transmission will go on immediately
            return;
        } // if

        // Send SREJ?
        if (m_SREJs.empty() == false) {
            // Send SREJs first
            l_Frame = PrepareSFrameSREJ();
        } // if
        
        // Send I-frame with data?
        if (l_Frame.IsEmpty() && (m_WaitQueueReliable.empty() == false) && (!m_bWaitForAck)) {
            l_Frame = PrepareIFrame();
            m_bWaitForAck = true;
            
            // I-frames carry an ACK
            m_bPeerRequiresAck = false;

            // Start retransmission timer
            auto self(shared_from_this());
            m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
            m_Timer.async_wait([this, self](const boost::system::error_code& ec) {
                if (!ec) {
                    // Send head of wait queue, maybe for a subsequent time.
                    m_bWaitForAck = false;
                    OpportunityForTransmission();
                } // if
            });
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
        m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_RAW, l_HDLCFrameBuffer, l_Frame.IsIFrame(), true, true); // not escaped
        m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_DISSECTED, l_Frame.Dissect(), l_Frame.IsIFrame(), true, true);
        m_SerialPortHandler->TransmitHDLCFrame(std::move(FrameGenerator::EscapeFrame(l_HDLCFrameBuffer)));
    } // if
}

Frame ProtocolState::PrepareIFrame() {
    // Fresh Payload to be sent is available.
    assert(m_WaitQueueReliable.empty() == false);
    m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_PAYLOAD, m_WaitQueueReliable.front(), true, true, true);

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
    m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_PAYLOAD, m_WaitQueueUnreliable.front(), false, true, true);

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
