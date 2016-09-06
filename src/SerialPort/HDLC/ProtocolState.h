/**
 * \file ProtocolState.h
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

#ifndef PROTOCOL_STATE_H
#define PROTOCOL_STATE_H

#include <vector>
#include <boost/asio.hpp>
#include <memory>
#include <deque>
#include "AliveState.h"
#include "Frame.h"
#include "FrameParser.h"
class ISerialPortHandler;

class ProtocolState: public std::enable_shared_from_this<ProtocolState> {
public:
    ProtocolState(std::shared_ptr<ISerialPortHandler> a_SerialPortHandler, boost::asio::io_service& a_IOService);
    
    void Start();
    void Stop();
    void Shutdown();

    void SendPayload(const std::vector<unsigned char> &a_Payload, bool a_bReliable);
    void TriggerNextHDLCFrame();
    void AddReceivedRawBytes(const unsigned char* a_Buffer, size_t a_Bytes);
    void InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame, bool a_bMessageInvalid);
    
    // Query state
    bool IsAlive() const { return m_AliveState->IsAlive(); }
    bool IsRunning() const  { return m_bStarted; }

private:
    // Internal helpers
    void Reset();
    void OpportunityForTransmission();
    Frame PrepareIFrame();
    Frame PrepareSFrameRR();
    Frame PrepareSFrameSREJ();
    Frame PrepareUFrameUI();
    Frame PrepareUFrameTEST();
    
    // Members
    bool m_bStarted;
    bool m_bAwaitsNextHDLCFrame;
    unsigned char m_SSeqOutgoing; // The sequence number we are going to use for the transmission of the next packet
    unsigned char m_RSeqIncoming; // The start of the RX window we offer our peer, defines which packets we expect
    
    // State of pending actions
    bool m_bSendProbe;
    bool m_bPeerStoppedFlow;        // RNR condition
    bool m_bPeerStoppedFlowNew;     // RNR condition
    bool m_bPeerStoppedFlowQueried; // RNR condition
    bool m_bPeerRequiresAck;
    bool m_bWaitForAck;
    std::deque<unsigned char> m_SREJs;
    
    // Parser and generator
    std::shared_ptr<ISerialPortHandler> m_SerialPortHandler;
    FrameParser m_FrameParser;
    
    // Wait queues
    std::deque<std::vector<unsigned char>> m_WaitQueueReliable;
    std::deque<std::vector<unsigned char>> m_WaitQueueUnreliable;
    
    // Alive state
    std::shared_ptr<AliveState> m_AliveState;
    
    // Timer
    boost::asio::deadline_timer m_Timer;
    bool m_bAliveReceivedSometing;
};

#endif // PROTOCOL_STATE_H
