/**
 * \file ClientHandler.cpp
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

#include "ClientHandler.h"
#include "ClientHandlerCollection.h"
#include "../SerialPort/SerialPortHandlerCollection.h"
#include "../SerialPort/SerialPortHandler.h"
#include "../../shared/PacketData.h"
#include "../../shared/PacketCtrl.h"

ClientHandler::ClientHandler(std::weak_ptr<ClientHandlerCollection> a_ClientHandlerCollection, boost::asio::ip::tcp::socket a_TCPSocket):
    m_ClientHandlerCollection(a_ClientHandlerCollection),
    m_TCPSocket(std::move(a_TCPSocket)) {
    m_Registered = false;
    m_eHDLCBuffer = HDLCBUFFER_NOTHING;
    m_bDeliverSent = false;
    m_bDeliverRcvd = false;
    m_bDeliverInvalidData = false;
    m_bSerialPortHandlerAwaitsPacket = false;
    m_PacketEndpoint = std::make_shared<PacketEndpoint>(m_TCPSocket);
    m_PacketEndpoint->SetOnDataCallback([this](std::shared_ptr<const PacketData> a_PacketData){ return OnDataReceived(a_PacketData); });
    m_PacketEndpoint->SetOnCtrlCallback([this](const PacketCtrl& a_PacketCtrl){ OnCtrlReceived(a_PacketCtrl); });
    m_PacketEndpoint->SetOnClosedCallback([this](){ OnClosed(); });
}

ClientHandler::~ClientHandler() {
    Stop();
}

void ClientHandler::DeliverBufferToClient(E_HDLCBUFFER a_eHDLCBuffer, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bValid, bool a_bWasSent) {
    // Check whether this buffer is of interest to this specific client
    bool l_bDeliver = (a_eHDLCBuffer == m_eHDLCBuffer);
    if ((a_bWasSent && !m_bDeliverSent) || (!a_bWasSent && !m_bDeliverRcvd)) {
        l_bDeliver = false;
    } // if
    
    if ((m_bDeliverInvalidData == false) && (a_bValid == false)) {
        l_bDeliver = false;
    } // if

    if (l_bDeliver) {
        auto l_Packet = PacketData::CreatePacket(a_Payload, a_bReliable, a_bValid, a_bWasSent);
        m_PacketEndpoint->Send(&l_Packet);
    } // if
}

void ClientHandler::UpdateSerialPortState(bool a_bAlive, bool a_bFlowControl, size_t a_LockHolders) {
    m_FlowGuard.UpdateSerialPortState(a_bFlowControl); // do not communicate
    bool l_bDeliverChangedState = false;
    l_bDeliverChangedState |= m_AliveGuard.UpdateSerialPortState(a_bAlive);
    l_bDeliverChangedState |= m_LockGuard.UpdateSerialPortState(a_LockHolders);
    if (l_bDeliverChangedState) {
        // The state of the serial port state changed. Communicate the new state to the client.
        auto l_Packet = PacketCtrl::CreatePortStatusResponse(m_AliveGuard.IsAlive(), m_LockGuard.IsLockedByOthers(), m_LockGuard.IsLockedBySelf());
        m_PacketEndpoint->Send(&l_Packet);
    } // if
}

void ClientHandler::QueryForPayload() {
    // Checks
    if (!m_Registered) {
        return;
    } // if

    // Deliver a pending incoming data packet, dependend on the state of packets accepted by the serial port handler.
    // If no suitable packet is pending, set a flag that allows immediate delivery of the next packet.
    if (m_PendingIncomingPacketData) {
        // A pending packet is available. Deliver it.
        if (TryToDeliverPendingPacket()) {
            // It was delivered, and we want to receive more
            m_PacketEndpoint->TriggerNextDataPacket();
        } // if
    } else {
        // no packet was waiting, but we want to receive more
        m_bSerialPortHandlerAwaitsPacket = true;
        m_PacketEndpoint->TriggerNextDataPacket();
    } // else
}

void ClientHandler::Start(std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection) {
    assert(m_Registered == false);
    if (auto lock = m_ClientHandlerCollection.lock()) {
        m_Registered = true;
        m_bSerialPortHandlerAwaitsPacket = true;
        lock->RegisterClientHandler(shared_from_this());
    } else {
        assert(false);
    } // else

    m_SerialPortHandlerCollection = a_SerialPortHandlerCollection;
    ReadSessionHeader1();
}

void ClientHandler::Stop() {
    m_SerialPortHandler.reset();
    if (m_Registered) {
        m_Registered = false;
        m_bSerialPortHandlerAwaitsPacket = false;
        if (m_PacketEndpoint->WasStarted() == false) {
            m_TCPSocket.cancel();
            m_TCPSocket.close();
        } else {
            m_PacketEndpoint->Close();
        } // else

        if (auto lock = m_ClientHandlerCollection.lock()) {
            lock->DeregisterClientHandler(shared_from_this());
        } // if
    } // if
}

void ClientHandler::ReadSessionHeader1() {
    boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, 3),[this](boost::system::error_code a_ErrorCode, std::size_t length) {
        if (a_ErrorCode == boost::asio::error::operation_aborted) return;
        if (!a_ErrorCode) {
            // Check version of the access protocol
            unsigned char l_Version = m_ReadBuffer[0];
            if (l_Version != 0x00) {
                // Unknown version of the access protocol
                std::cerr << "Unsupported access protocol version rejected: " << (int)l_Version << std::endl;
                Stop();
                return;
            } // if
            
            // Check service access point specifier: type of data
            unsigned char l_SAP = m_ReadBuffer[1];
            switch (l_SAP & 0xF0) {
            case 0x00: {
                m_eHDLCBuffer = HDLCBUFFER_PAYLOAD;
                break;
            }
            case 0x10: {
                m_eHDLCBuffer = HDLCBUFFER_PORT_STATUS;
                break;
            }
            case 0x20: {
                m_eHDLCBuffer = HDLCBUFFER_PAYLOAD;
                break;
            }
            case 0x30: {
                m_eHDLCBuffer = HDLCBUFFER_RAW;
                break;
            }
            case 0x40: {
                m_eHDLCBuffer = HDLCBUFFER_DISSECTED;
                break;
            }
            default:
                // Unknown session type
                std::cerr << "Unknown session type rejected: " << (int)(l_SAP & 0xF0) << std::endl;
                Stop();
                return;
            } // switch
            
            // Check service access point specifier: reserved bit
            if (l_SAP & 0x08) {
                // The reserved bit was set... aborting
                std::cerr << "Invalid reserved bit within SAP specifier of session header: " << (int)l_SAP << std::endl;
                Stop();
                return;
            } // if
            
            // Check service access point specifier: invalids, deliver sent, and deliver rcvd
            m_bDeliverInvalidData = (l_SAP & 0x04); 
            m_bDeliverSent = (l_SAP & 0x02);
            m_bDeliverRcvd = (l_SAP & 0x01); 

            // Now read the name of the serial port
            ReadSessionHeader2(m_ReadBuffer[2]);
        } else {
            std::cerr << "TCP READ ERROR HEADER1:" << a_ErrorCode << std::endl;
            Stop();
        } // else
    });
}

void ClientHandler::ReadSessionHeader2(unsigned char a_BytesUSB) {
    boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, a_BytesUSB),[this](boost::system::error_code a_ErrorCode, std::size_t length) {
        if (a_ErrorCode == boost::asio::error::operation_aborted) return;
        auto self(shared_from_this());
        if (!a_ErrorCode) {
            // Now we know the USB port
            std::string l_UsbPortString;
            l_UsbPortString.append((char*)m_ReadBuffer, length);
            m_SerialPortHandlerStopper = m_SerialPortHandlerCollection->GetSerialPortHandler(l_UsbPortString, self);
            if (m_SerialPortHandlerStopper) {
                m_SerialPortHandler = (*m_SerialPortHandlerStopper.get());
                m_LockGuard.Init(m_SerialPortHandler);
                m_SerialPortHandler->PropagateSerialPortState(); // May sent initial port status message

                // Start the PacketEndpoint. It takes full control over the TCP socket.
                m_PacketEndpoint->Start();
            } else {
                Stop();
            } // else
        } else {
            std::cerr << "TCP READ ERROR:" << a_ErrorCode << std::endl;
            Stop();
        } // else
    });
}

bool ClientHandler::TryToDeliverPendingPacket() {
    // Checks
    assert(m_PendingIncomingPacketData);
    
    // Check if this type of data packet it is currently accepted
    if ((m_PendingIncomingPacketData->GetReliable() == false) || (m_FlowGuard.IsFlowSuspended() == false)) {
        // Deliver the pending packet and re-enable the TCP receiver
        m_SerialPortHandler->DeliverPayloadToHDLC(m_PendingIncomingPacketData->GetData(), m_PendingIncomingPacketData->GetReliable());
        m_PendingIncomingPacketData.reset();
        return true; // Awaits next packet
    } else {
        // Stall the TCP receiver
        return false;
    } // else
}

bool ClientHandler::OnDataReceived(std::shared_ptr<const PacketData> a_PacketData) {
    // Checks
    assert(a_PacketData);
    assert(!m_PendingIncomingPacketData);
    
    // Store the incoming packet, but try to deliver it now
    m_PendingIncomingPacketData = a_PacketData;
    if (m_bSerialPortHandlerAwaitsPacket) {
        m_bSerialPortHandlerAwaitsPacket = false;
        if (TryToDeliverPendingPacket()) {
            // Delivery successful, but for now, we must not deliver more. However, we can receive until we have the next packet
            return true;
        } // if
        
        // We were not able to deliver the incoming packet
    } // if
    
    // Stall the receiver
    return false;
}

void ClientHandler::OnCtrlReceived(const PacketCtrl& a_PacketCtrl) {
    // Check control packet: suspend / resume? Port kill request?
    switch(a_PacketCtrl.GetPacketType()) {
        case PacketCtrl::CTRL_TYPE_PORT_STATUS: {
            if (a_PacketCtrl.GetDesiredLockState()) {
                // Acquire a lock, if not already held. On acquisition, suspend the serial port
                m_LockGuard.AcquireLock();
            } else {
                // Release lock and resume serial port
                m_LockGuard.ReleaseLock();
            } // else
            break;
        }
        case PacketCtrl::CTRL_TYPE_ECHO: {
            // Respond with an echo reply control packet: simply send it back
            m_PacketEndpoint->Send(&a_PacketCtrl);
            break;
        }
        case PacketCtrl::CTRL_TYPE_PORT_KILL: {
            // Kill the serial port immediately and detach all related clients
            m_SerialPortHandler->Stop();
            break;
        }
        default:
            break;
    } // switch
}

void ClientHandler::OnClosed() {
    Stop();
}
