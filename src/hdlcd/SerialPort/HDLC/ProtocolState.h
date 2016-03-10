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

#ifndef PROTOCOL_STATE_H
#define PROTOCOL_STATE_H

#include <vector>
#include <boost/asio.hpp>
#include <memory>
#include <deque>
#include "AliveState.h"
#include "Frame.h"
#include "FrameParser.h"
class SerialPortHandler;

class ProtocolState: public std::enable_shared_from_this<ProtocolState> {
public:
    ProtocolState(std::shared_ptr<SerialPortHandler> a_SerialPortHandler, boost::asio::io_service& a_IOService);
    
    void Start();
    void Stop();
    void Shutdown();

    void SendPayload(const std::vector<unsigned char> &a_Payload, bool a_bReliable);
    void TriggerNextHDLCFrame();
    void AddReceivedRawBytes(const unsigned char* a_Buffer, size_t a_Bytes);
    void InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame, bool a_bMessageValid);
    
    // Query state
    bool IsAlive() const { return m_AliveState->IsAlive(); }
    bool IsFlowSuspended() const { return (m_bPeerStoppedFlow || !IsAlive()); }

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
    unsigned char m_SSeqIncoming; // The sequence number we expect our peer to use for the next packet directed to us
    unsigned char m_RSeqIncoming; // The start of the RX window we offer our peer, defines which packets we expect
    
    // State of pending actions
    bool m_bSendProbe;
    bool m_bPeerStoppedFlow;      // RNR condition
    bool m_bPeerRequiresAck;
    bool m_bWaitForAck;
    std::deque<unsigned char> m_SREJs;
    
    // Parser and generator
    std::shared_ptr<SerialPortHandler> m_SerialPortHandler;
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
