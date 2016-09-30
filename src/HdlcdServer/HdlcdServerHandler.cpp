/**
 * \file HdlcdServerHandler.cpp
 * \brief 
 *
 * The HDLC Deamon implements the HDLC protocol to easily talk to devices connected via serial communications.
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

#include "HdlcdServerHandler.h"
#include "HdlcdServerHandlerCollection.h"
#include "SerialPortHandlerCollection.h"
#include "SerialPortHandler.h"
#include "HdlcdPacketData.h"
#include "HdlcdPacketCtrl.h"
#include "HdlcdPacketEndpoint.h"
#include "HdlcdSessionHeader.h"
#include "FrameEndpoint.h"
#include <utility>

HdlcdServerHandler::HdlcdServerHandler(boost::asio::io_service& a_IOService, std::weak_ptr<HdlcdServerHandlerCollection> a_HdlcdServerHandlerCollection, boost::asio::ip::tcp::socket& a_TcpSocket): m_IOService(a_IOService), m_HdlcdServerHandlerCollection(a_HdlcdServerHandlerCollection) {
    // Initialize members
    m_Registered = false;
    m_bDeliverInitialState = true;
    m_eBufferType = BUFFER_TYPE_UNSET;
    m_bDeliverSent = false;
    m_bDeliverRcvd = false;
    m_bDeliverInvalidData = false;
    m_bSerialPortHandlerAwaitsPacket = false;
    
    // Prepare frame endpoint
    m_FrameEndpoint = std::make_shared<FrameEndpoint>(a_IOService, a_TcpSocket);
    m_FrameEndpoint->RegisterFrameFactory(0x00, []()->std::shared_ptr<Frame>{ return HdlcdSessionHeader::CreateDeserializedFrame(); });
    m_FrameEndpoint->SetOnFrameCallback([this](std::shared_ptr<Frame> a_Frame)->bool{ return OnFrame(a_Frame); });
    m_FrameEndpoint->SetOnClosedCallback ([this](){ OnClosed(); });
}

void HdlcdServerHandler::DeliverBufferToClient(E_BUFFER_TYPE a_eBufferType, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bInvalid, bool a_bWasSent) {
    // Check whether this buffer is of interest to this specific client
    bool l_bDeliver = (a_eBufferType == m_eBufferType);
    if ((a_bWasSent && !m_bDeliverSent) || (!a_bWasSent && !m_bDeliverRcvd)) {
        l_bDeliver = false;
    } // if
    
    if ((m_bDeliverInvalidData == false) && (a_bInvalid)) {
        l_bDeliver = false;
    } // if

    if (l_bDeliver) {
        m_PacketEndpoint->Send(HdlcdPacketData::CreatePacket(a_Payload, a_bReliable, a_bInvalid, a_bWasSent));
    } // if
}

void HdlcdServerHandler::UpdateSerialPortState(bool a_bAlive, size_t a_LockHolders) {
    bool l_bDeliverChangedState = m_bDeliverInitialState;
    m_bDeliverInitialState = false;
    l_bDeliverChangedState |= m_AliveGuard.UpdateSerialPortState(a_bAlive);
    l_bDeliverChangedState |= m_LockGuard.UpdateSerialPortState(a_LockHolders);
    if (l_bDeliverChangedState) {
        // The state of the serial port state changed. Communicate the new state to the client.
        m_PacketEndpoint->Send(HdlcdPacketCtrl::CreatePortStatusResponse(m_AliveGuard.IsAlive(), m_LockGuard.IsLockedByOthers(), m_LockGuard.IsLockedBySelf()));
    } // if
}

void HdlcdServerHandler::QueryForPayload(bool a_bQueryReliable, bool a_bQueryUnreliable) {
    // Checks
    assert(a_bQueryReliable || a_bQueryUnreliable);
    if (!m_Registered) {
        return;
    } // if

    // Deliver a pending incoming data packet, dependend on the state of packets accepted by the serial port handler.
    // If no suitable packet is pending, set a flag that allows immediate delivery of the next packet.
    if (m_PendingIncomingPacketData) {
        bool l_bDeliver = (a_bQueryReliable   &&  m_PendingIncomingPacketData->GetReliable());
        l_bDeliver     |= (a_bQueryUnreliable && !m_PendingIncomingPacketData->GetReliable());
        if (l_bDeliver) {
            m_SerialPortHandler->DeliverPayloadToHDLC(m_PendingIncomingPacketData->GetData(), m_PendingIncomingPacketData->GetReliable());
            m_PendingIncomingPacketData.reset();
            m_PacketEndpoint->TriggerNextDataPacket();
        } // if
    } else {
        // No packet was pending, but we want to receive more!
        m_bSerialPortHandlerAwaitsPacket = true;
        m_PacketEndpoint->TriggerNextDataPacket();
    } // else
}

void HdlcdServerHandler::Start(std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection) {
    assert(m_Registered == false);
    assert(a_SerialPortHandlerCollection);
    m_SerialPortHandlerCollection = a_SerialPortHandlerCollection;
    if (auto lock = m_HdlcdServerHandlerCollection.lock()) {
        m_Registered = true;
        m_bSerialPortHandlerAwaitsPacket = true;
        lock->RegisterHdlcdServerHandler(shared_from_this());
    } else {
        assert(false);
    } // else

    // Start waiting for the session header
    m_FrameEndpoint->Start();
}

void HdlcdServerHandler::Stop() {
    // Keep this object alive
    auto self(shared_from_this());
    m_SerialPortHandler.reset();
    if (m_Registered) {
        m_Registered = false;
        m_bSerialPortHandlerAwaitsPacket = false;
        if (m_PacketEndpoint) {
            assert(!m_FrameEndpoint);
            m_PacketEndpoint->Close();
            m_PacketEndpoint.reset();
        } else {    
            m_FrameEndpoint->Shutdown();
            m_FrameEndpoint->Close();
            m_FrameEndpoint.reset();
        } // else

        if (auto lock = m_HdlcdServerHandlerCollection.lock()) {
            lock->DeregisterHdlcdServerHandler(self);
        } // if
    } // if
}

bool HdlcdServerHandler::OnFrame(const std::shared_ptr<Frame> a_Frame) {
    // Checks
    assert(a_Frame);
    assert(m_FrameEndpoint);
    assert(!m_PacketEndpoint);

    // Parse the session header
    auto l_HdlcdSessionHeader = std::dynamic_pointer_cast<HdlcdSessionHeader>(a_Frame);
    if (l_HdlcdSessionHeader) {
        // The session header is now available. Check service access point specifier: type of data
        uint8_t l_SAP = l_HdlcdSessionHeader->GetServiceAccessPointSpecifier();
        switch (l_SAP & 0xF0) {
        case 0x00: {
            m_eBufferType = BUFFER_TYPE_PAYLOAD;
            break;
        }
        case 0x10: {
            m_eBufferType = BUFFER_TYPE_PORT_STATUS;
            break;
        }
        case 0x20: {
            m_eBufferType = BUFFER_TYPE_PAYLOAD;
            break;
        }
        case 0x30: {
            m_eBufferType = BUFFER_TYPE_RAW;
            break;
        }
        case 0x40: {
            m_eBufferType = BUFFER_TYPE_DISSECTED;
            break;
        }
        default:
            // Unknown session type
            std::cerr << "Unknown session type rejected: " << (int)(l_SAP & 0xF0) << std::endl;
            Stop();
            return false;
        } // switch
        
        // Check service access point specifier: reserved bit
        if (l_SAP & 0x08) {
            // The reserved bit was set... aborting
            std::cerr << "Invalid reserved bit within SAP specifier of session header: " << (int)l_SAP << std::endl;
            Stop();
            return false;
        } // if
        
        // Check service access point specifier: invalids, deliver sent, and deliver rcvd
        m_bDeliverInvalidData = (l_SAP & 0x04); 
        m_bDeliverSent        = (l_SAP & 0x02);
        m_bDeliverRcvd        = (l_SAP & 0x01);
        
        // Start the PacketEndpoint. It takes full control over the TCP socket.
        m_PacketEndpoint = std::make_shared<HdlcdPacketEndpoint>(m_IOService, m_FrameEndpoint);
        m_PacketEndpoint->SetOnDataCallback([this](std::shared_ptr<const HdlcdPacketData> a_PacketData){ return OnDataReceived(a_PacketData); });
        m_PacketEndpoint->SetOnCtrlCallback([this](const HdlcdPacketCtrl& a_PacketCtrl){ OnCtrlReceived(a_PacketCtrl); });
        m_PacketEndpoint->SetOnClosedCallback([this](){ OnClosed(); });
        m_FrameEndpoint.reset();
        auto l_SerialPortHandlerStopper = m_SerialPortHandlerCollection->GetSerialPortHandler(l_HdlcdSessionHeader->GetSerialPortName(), shared_from_this());
        if (l_SerialPortHandlerStopper) {
            m_SerialPortHandlerStopper = l_SerialPortHandlerStopper;
            m_SerialPortHandler = (*m_SerialPortHandlerStopper.get());
            m_LockGuard.Init(m_SerialPortHandler);
            m_SerialPortHandler->PropagateSerialPortState(); // Sends initial port status message
            m_PacketEndpoint->Start();
        } else {
            // This object is dead now! -> Close() was already called by the SerialPortHandler
        } // else
    } else {
        // Instead of a session header we received junk! This is impossible.
        assert(false);
        Stop();
    } // else
    
    // In each case: stall the receiver! The HdlcdPacketEndpoint continues...
    return false;
}

bool HdlcdServerHandler::OnDataReceived(std::shared_ptr<const HdlcdPacketData> a_PacketData) {
    // Checks
    assert(a_PacketData);
    assert(!m_PendingIncomingPacketData);
    
    // Store the incoming packet, but try to deliver it now
    m_PendingIncomingPacketData = a_PacketData;
    if (m_bSerialPortHandlerAwaitsPacket) {
        // One packet can be delivered, regardless of its reliablility status and the kind of packets that are accepted.
        m_bSerialPortHandlerAwaitsPacket = false;
        m_SerialPortHandler->DeliverPayloadToHDLC(m_PendingIncomingPacketData->GetData(), m_PendingIncomingPacketData->GetReliable());
        m_PendingIncomingPacketData.reset();
        return true; // continue receiving, we stall with the next data packet
    } // if
    
    // Stall the receiver now!
    return false;
}

void HdlcdServerHandler::OnCtrlReceived(const HdlcdPacketCtrl& a_PacketCtrl) {
    // Check control packet: suspend / resume? Port kill request?
    switch(a_PacketCtrl.GetPacketType()) {
        case HdlcdPacketCtrl::CTRL_TYPE_PORT_STATUS: {
            if (a_PacketCtrl.GetDesiredLockState()) {
                // Acquire a lock, if not already held. On acquisition, suspend the serial port
                m_LockGuard.AcquireLock();
            } else {
                // Release lock and resume serial port
                m_LockGuard.ReleaseLock();
            } // else
            break;
        }
        case HdlcdPacketCtrl::CTRL_TYPE_ECHO: {
            // Respond with an echo reply control packet: simply send it back
            m_PacketEndpoint->Send(a_PacketCtrl);
            break;
        }
        case HdlcdPacketCtrl::CTRL_TYPE_PORT_KILL: {
            // Kill the serial port immediately and detach all related clients
            m_SerialPortHandler->Stop();
            break;
        }
        default:
            break;
    } // switch
}

void HdlcdServerHandler::OnClosed() {
    Stop();
}
