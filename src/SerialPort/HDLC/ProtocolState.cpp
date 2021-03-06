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
#include "FrameGenerator.h"
#include "ISerialPortHandler.h"

ProtocolState::ProtocolState(std::shared_ptr<ISerialPortHandler> a_SerialPortHandler, boost::asio::io_service& a_IOService): m_SerialPortHandler(a_SerialPortHandler), m_FrameParser(*this), m_Timer(a_IOService) {
    // Initialize alive state helper
    m_AliveState = std::make_shared<AliveState>(a_IOService);
    m_AliveState->SetSendProbeCallback([this]() {
        m_bSendProbe = true;
        if (m_SerialPortHandler) {
            OpportunityForTransmission();
        } else {
            // Already closed or shutdown was called
        } // else
    });
    m_AliveState->SetChangeBaudrateCallback([this]() {
        if (m_SerialPortHandler) {
            m_SerialPortHandler->ChangeBaudRate();
            m_SerialPortHandler->PropagateSerialPortState();
        } else {
            // Already closed or shutdown was called
        } // else
    });
    
    Reset();
}

void ProtocolState::Reset() {
    m_AliveState->Stop();
    m_Timer.cancel();
    m_bStarted = false;
    m_bAwaitsNextHDLCFrame = true;
    m_SSeqOutgoing = 0;
    m_RSeqIncoming = 0;
    m_bSendProbe = false;
    m_bPeerStoppedFlow = false;
    m_bPeerStoppedFlowNew = false;
    m_bPeerStoppedFlowQueried = false;
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
        m_SerialPortHandler->PropagateSerialPortState();
    } // if
}

void ProtocolState::Shutdown() {
    if (m_bStarted) {
        // Stop the state machine, but do not emit any subsequent events
        Reset();
        m_SerialPortHandler.reset();
    } // if
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

void ProtocolState::InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const HdlcFrame& a_HdlcFrame, bool a_bMessageInvalid) {
    // Checks
    if (!m_bStarted) {
        return;
    } // if

    // Deliver raw frame to clients that have interest
    if (m_SerialPortHandler->RequiresBufferType(BUFFER_TYPE_RAW)) {
        m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_RAW, a_Payload, false, a_bMessageInvalid, false); // not escaped
    } // if
    
    if (m_SerialPortHandler->RequiresBufferType(BUFFER_TYPE_DISSECTED)) {
        m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_DISSECTED, a_HdlcFrame.Dissect(), false, a_bMessageInvalid, false);
    } // if
    
    // Stop here if the frame was considered broken
    if (a_bMessageInvalid) {
        return;
    } // if
    
    // A valid frame was received
    if (m_AliveState->OnFrameReceived()) {
        m_SerialPortHandler->PropagateSerialPortState();
    } // if
    
    // Go ahead interpreting the frame we received
    if (a_HdlcFrame.HasPayload()) {
        // I-Frame or U-Frame with UI
        if (m_SerialPortHandler->RequiresBufferType(BUFFER_TYPE_PAYLOAD)) {
            m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_PAYLOAD, a_HdlcFrame.GetPayload(), a_HdlcFrame.IsIFrame(), a_bMessageInvalid, false);
        } // if
        
        // If it is an I-Frame, the data may have to be acked
        if (a_HdlcFrame.IsIFrame()) {
            if (m_RSeqIncoming != a_HdlcFrame.GetSSeq()) {
                m_SREJs.clear();
                for (unsigned char it = m_RSeqIncoming; (it & 0x07) != a_HdlcFrame.GetSSeq(); ++it) {
                    m_SREJs.emplace_back(it);
                } // for
            } // if

            m_RSeqIncoming = ((a_HdlcFrame.GetSSeq() + 1) & 0x07);
            m_bPeerRequiresAck = true;
        } // if
    } // if
    
    // Check the various types of ACKs and NACKs
    if ((a_HdlcFrame.IsIFrame()) || (a_HdlcFrame.IsSFrame())) {
        if ((a_HdlcFrame.IsIFrame()) || (a_HdlcFrame.GetHDLCFrameType() == HdlcFrame::HDLC_FRAMETYPE_S_RR)) {
            if ((m_bPeerStoppedFlow) && (a_HdlcFrame.GetHDLCFrameType() == HdlcFrame::HDLC_FRAMETYPE_S_RR)) {
                // The peer restarted the flow: RR clears RNR condition
                m_bPeerStoppedFlow = false;
                m_bWaitForAck = false;
                m_Timer.cancel();
            } // if

            // Currently, we send only one I-frame at a time and wait until the respective ACK is received
            if ((m_bWaitForAck) && (a_HdlcFrame.GetRSeq() == ((m_SSeqOutgoing + 1) & 0x07))) {
                // We found the respective sequence number to the last transmitted I-frame
                m_bWaitForAck = false;
                m_Timer.cancel();
                m_WaitQueueReliable.pop_front();
            } // if
            
            m_SSeqOutgoing = a_HdlcFrame.GetRSeq();
        } else if (a_HdlcFrame.GetHDLCFrameType() == HdlcFrame::HDLC_FRAMETYPE_S_RNR) {
            // The peer wants us to stop sending subsequent data
            if (!m_bPeerStoppedFlow) {
                // Start periodical query
                m_bPeerStoppedFlow = true;
                m_bPeerStoppedFlowNew = true;
                m_bPeerStoppedFlowQueried = false;
            } // if

            if (m_bWaitForAck) {
                m_bWaitForAck = false;
                m_Timer.cancel();
                if (a_HdlcFrame.GetRSeq() == ((m_SSeqOutgoing + 1) & 0x07)) {
                    // We found the respective sequence number to the last transmitted I-frame
                    m_WaitQueueReliable.pop_front();
                } // if
            } // if
            
            // Now we know which SeqNr the peer awaits next... after the RNR condition was cleared
            m_SSeqOutgoing = a_HdlcFrame.GetRSeq();
        } else if (a_HdlcFrame.GetHDLCFrameType() == HdlcFrame::HDLC_FRAMETYPE_S_REJ) {
            if (m_bPeerStoppedFlow) {
                // The peer restarted the flow: REJ clears RNR condition
                m_bPeerStoppedFlow = false;
                m_bWaitForAck = false;
                m_Timer.cancel();
            } // if
            
            // The peer requests for go-back-N. We have to retransmit all affected packets, but not with this version of HDLC.
            if (a_HdlcFrame.GetRSeq() == m_SSeqOutgoing) {
                // We found the respective sequence number to the last transmitted I-frame
                m_bWaitForAck = false;
                m_Timer.cancel();
            } // if
            
            m_SSeqOutgoing = ((a_HdlcFrame.GetRSeq() + 0x07) & 0x07);
        } else {
            assert(a_HdlcFrame.GetHDLCFrameType() == HdlcFrame::HDLC_FRAMETYPE_S_SREJ);
            if (m_bPeerStoppedFlow) {
                // The peer restarted the flow: SREJ clears RNR condition
                m_bPeerStoppedFlow = false;
                m_bWaitForAck = false;
                m_Timer.cancel();
            } // if
            
            // The peer requests for the retransmission of a single segment with a specific sequence number
            // This cannot be implemented using this reduced version of HDLC!
            if ((m_bWaitForAck) && (a_HdlcFrame.GetRSeq() == m_SSeqOutgoing)) {
                // We found the respective sequence number to the last transmitted I-frame.
                // In this version of HDLC, this should not happen!
                m_bWaitForAck = false;
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
    
    HdlcFrame l_HdlcFrame;
    if (m_bSendProbe) {
        // The correct baud rate setting is unknown yet, or it has to be checked again. Send an U-TEST frame.
        m_bSendProbe = false;
        l_HdlcFrame = PrepareUFrameTEST();
    } // if
    
    if (l_HdlcFrame.IsEmpty() && m_AliveState->IsAlive()) {
        // The serial link to the device is alive. Send all outstanding S-SREJs first.
        if (m_SREJs.empty() == false) {
            // Send SREJs first
            l_HdlcFrame = PrepareSFrameSREJ();
        } // if
        
        // Check if packets are waiting for reliable transmission
        if (l_HdlcFrame.IsEmpty() && (m_WaitQueueReliable.empty() == false) && (!m_bWaitForAck) && (!m_bPeerStoppedFlow)) {
            // Send an I-Frame now
            l_HdlcFrame = PrepareIFrame();
            m_bWaitForAck = true;
            
            // I-frames carry an ACK
            m_bPeerRequiresAck = false;

            // Start retransmission timer
            auto self(shared_from_this());
            m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
            m_Timer.async_wait([this, self](const boost::system::error_code& ec) {
                if (!ec) {
                    // Send the head element of the wait queue again
                    m_bWaitForAck = false;
                    OpportunityForTransmission();
                } // if
            });
        } // if
        
        // Send outstanding RR?
        if (l_HdlcFrame.IsEmpty() && ((m_bPeerRequiresAck) || m_bPeerStoppedFlowNew)) {
            bool l_bStartTimer = false;
            if (m_bPeerStoppedFlowNew) {
                m_bPeerStoppedFlowNew = false;
                l_bStartTimer = true;
            } else {
                // Prepare RR
                l_HdlcFrame = PrepareSFrameRR();
                m_bPeerRequiresAck = false;
                if (m_bPeerStoppedFlow && !m_bPeerStoppedFlowQueried) {
                    // During the RNR confition at the peer, we query it periodically
                    l_HdlcFrame.SetPF(true); // This is a hack due to missing command/response support
                    m_bPeerStoppedFlowQueried = true;
                    l_bStartTimer = true;
                } // if
            } // else
            
            if (l_bStartTimer) {
                m_Timer.cancel();    
                auto self(shared_from_this());
                m_Timer.expires_from_now(boost::posix_time::milliseconds(500));
                m_Timer.async_wait([this, self](const boost::system::error_code& ec) {
                    if (!ec) {
                        if (m_bPeerStoppedFlow) {
                            m_bPeerRequiresAck = true;
                            m_bPeerStoppedFlowQueried = false;
                            OpportunityForTransmission();
                        } // if
                    } // if
                });
            } // if
        } // if        
        
        // Check if packets are waiting for unreliable transmission
        if (l_HdlcFrame.IsEmpty() && (m_WaitQueueUnreliable.empty() == false)) {
            l_HdlcFrame = PrepareUFrameUI();
            m_WaitQueueUnreliable.pop_front();
        } // if
        
        // If there is nothing to send, try to fill the wait queues, but only if necessary.
        if (l_HdlcFrame.IsEmpty()) {
            // These expressions are the result of some boolean logic
            bool l_bQueryReliable   = (m_WaitQueueUnreliable.empty() &&  m_WaitQueueReliable.empty() && (!m_bPeerStoppedFlow));
            bool l_bQueryUnreliable = (m_WaitQueueUnreliable.empty() && (m_WaitQueueReliable.empty() ||   m_bPeerStoppedFlow));
            if (l_bQueryReliable || l_bQueryUnreliable) {
                m_SerialPortHandler->QueryForPayload(l_bQueryReliable, l_bQueryUnreliable);
            } // if
        } // if        
    } // if

    if (l_HdlcFrame.IsEmpty() == false) {
        // Deliver unescaped frame to clients that have interest
        m_bAwaitsNextHDLCFrame = false;
        auto l_HDLCFrameBuffer = FrameGenerator::SerializeFrame(l_HdlcFrame);
        if (m_SerialPortHandler->RequiresBufferType(BUFFER_TYPE_RAW)) {
            m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_RAW, l_HDLCFrameBuffer, l_HdlcFrame.IsIFrame(), false, true); // not escaped
        } // if
        
        if (m_SerialPortHandler->RequiresBufferType(BUFFER_TYPE_DISSECTED)) {
            m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_DISSECTED, l_HdlcFrame.Dissect(), l_HdlcFrame.IsIFrame(), false, true);
        } // if
        
        m_SerialPortHandler->TransmitHDLCFrame(std::move(FrameGenerator::EscapeFrame(l_HDLCFrameBuffer)));
    } // if
}

HdlcFrame ProtocolState::PrepareIFrame() {
    // Fresh Payload to be sent is available.
    assert(m_WaitQueueReliable.empty() == false);
    if (m_SerialPortHandler->RequiresBufferType(BUFFER_TYPE_PAYLOAD)) {
        m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_PAYLOAD, m_WaitQueueReliable.front(), true, false, true);
    } // if

    // Prepare I-Frame    
    HdlcFrame l_HdlcFrame;
    l_HdlcFrame.SetAddress(0x30);
    l_HdlcFrame.SetHDLCFrameType(HdlcFrame::HDLC_FRAMETYPE_I);
    l_HdlcFrame.SetPF(false);
    l_HdlcFrame.SetSSeq(m_SSeqOutgoing);
    l_HdlcFrame.SetRSeq(m_RSeqIncoming);
    l_HdlcFrame.SetPayload(m_WaitQueueReliable.front());
    return(l_HdlcFrame);
}

HdlcFrame ProtocolState::PrepareSFrameRR() {
    HdlcFrame l_HdlcFrame;
    l_HdlcFrame.SetAddress(0x30);
    l_HdlcFrame.SetHDLCFrameType(HdlcFrame::HDLC_FRAMETYPE_S_RR);
    l_HdlcFrame.SetPF(false);
    l_HdlcFrame.SetRSeq(m_RSeqIncoming);
    return(l_HdlcFrame);
}

HdlcFrame ProtocolState::PrepareSFrameSREJ() {
    HdlcFrame l_HdlcFrame;
    l_HdlcFrame.SetAddress(0x30);
    l_HdlcFrame.SetHDLCFrameType(HdlcFrame::HDLC_FRAMETYPE_S_SREJ);
    l_HdlcFrame.SetPF(false);
    l_HdlcFrame.SetRSeq(m_SREJs.front());
    m_SREJs.pop_front();
    return(l_HdlcFrame);
}

HdlcFrame ProtocolState::PrepareUFrameUI() {
    assert(m_WaitQueueUnreliable.empty() == false);
    m_SerialPortHandler->DeliverBufferToClients(BUFFER_TYPE_PAYLOAD, m_WaitQueueUnreliable.front(), false, false, true);

    // Prepare UI-Frame
    HdlcFrame l_HdlcFrame;
    l_HdlcFrame.SetAddress(0x30);
    l_HdlcFrame.SetHDLCFrameType(HdlcFrame::HDLC_FRAMETYPE_U_UI);
    l_HdlcFrame.SetPF(false);
    l_HdlcFrame.SetPayload(m_WaitQueueUnreliable.front());
    return(l_HdlcFrame);
}

HdlcFrame ProtocolState::PrepareUFrameTEST() {
    HdlcFrame l_HdlcFrame;
    l_HdlcFrame.SetAddress(0x30);
    l_HdlcFrame.SetHDLCFrameType(HdlcFrame::HDLC_FRAMETYPE_U_TEST);
    l_HdlcFrame.SetPF(false);
    return(l_HdlcFrame);
}
