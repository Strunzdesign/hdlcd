/**
 * \file PacketEndpoint.h
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

#ifndef PACKET_ENDPOINT_H
#define PACKET_ENDPOINT_H

#include <deque>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include "../shared/PacketData.h"
#include "../shared/PacketCtrl.h"

class PacketEndpoint: public std::enable_shared_from_this<PacketEndpoint> {
public:
    PacketEndpoint(boost::asio::ip::tcp::socket& a_TCPSocket): m_KeepAliveTimer(a_TCPSocket.get_io_service()), m_TCPSocket(a_TCPSocket) {
        m_SEPState = SEPSTATE_DISCONNECTED;
        m_bWriteInProgress = false;
        m_bShutdown = false;
        m_bStarted = false;
        m_bStopped = false;
        m_bReceiving = false;
    }
    
    ~PacketEndpoint() {
        m_OnDataCallback = NULL;
        m_OnCtrlCallback = NULL;
        m_OnClosedCallback = NULL;
        Close();
    }
    
    bool WasStarted() const {
        return m_bStarted;
    }
            
    // Callback methods
    void SetOnDataCallback(std::function<bool(std::shared_ptr<const PacketData> a_PacketData)> a_OnDataCallback) {
        m_OnDataCallback = a_OnDataCallback;
    }
    
    void SetOnCtrlCallback(std::function<void(const PacketCtrl& a_PacketCtrl)> a_OnCtrlCallback) {
        m_OnCtrlCallback = a_OnCtrlCallback;
    }
    
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }
    
    bool Send(const Packet* a_Packet, std::function<void()> a_OnSendDoneCallback = std::function<void()>()) {
        assert(a_Packet != NULL);
        if (m_SEPState == SEPSTATE_SHUTDOWN) {
            return false;
        } // if

        // TODO: check size of the queue. If it reaches a specific limit: kill the socket to prevent DoS attacks
        if (m_SendQueue.size() >= 10) {
            if (a_OnSendDoneCallback) {
                a_OnSendDoneCallback();
            } // if

            return false;
        } // if
        
        m_SendQueue.emplace_back(std::make_pair(std::move(a_Packet->Serialize()), a_OnSendDoneCallback));
        bool write_in_progress = !m_SendQueue.empty();
        if ((!m_bWriteInProgress) && (!m_SendQueue.empty()) && (m_SEPState == SEPSTATE_CONNECTED)) {
            do_write();
        } // if
        
        return true;
    }
    
    void Start() {
        assert(m_bStarted == false);
        assert(m_bStopped == false);
        assert(m_SEPState == SEPSTATE_DISCONNECTED);
        assert(m_bWriteInProgress == false);
        assert(m_bReceiving == false);
        m_bStarted = true;
        m_SEPState = SEPSTATE_CONNECTED;
        TriggerNextDataPacket();
        StartKeepAliveTimer();
        if (!m_SendQueue.empty()) {
            do_write();
        } // if
    }

    void Shutdown() {
        m_bShutdown = true;
        m_KeepAliveTimer.cancel();
    }

    void Close() {
        if (m_bStarted && (!m_bStopped)) {
            m_bStopped = true;
            m_bReceiving = false;
            m_KeepAliveTimer.cancel();
            m_TCPSocket.cancel();
            m_TCPSocket.close();
            if (m_OnClosedCallback) {
                m_OnClosedCallback();
            } // if
        } // if
    }
    
    void TriggerNextDataPacket() {
        // Checks
        if (m_bReceiving) {
            return;
        } // if
        
        if ((m_bStarted) && (!m_bStopped) && (m_SEPState == SEPSTATE_CONNECTED)) {
            // Start reading the next packet
            ReadType();
        } // if
    }

private:
    void StartKeepAliveTimer() {
        auto self(shared_from_this());
        m_KeepAliveTimer.expires_from_now(boost::posix_time::minutes(1));
        m_KeepAliveTimer.async_wait([this, self](const boost::system::error_code& a_ErrorCode) {
            if (!a_ErrorCode) {
                // Periodically send keep alive packet, but do not congest the TCP socket
                if (m_SendQueue.empty()) {
                    auto l_Packet = PacketCtrl::CreateKeepAliveRequest();
                    Send(&l_Packet);
                    StartKeepAliveTimer();
                } // if
            } // if
        });
    }

    void ReadType() {
        m_bReceiving = true;
        auto self(shared_from_this());
        if (m_bStopped) return;
        assert(!m_IncomingPacket);
        boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, 1),
                                [this, self](boost::system::error_code a_ErrorCode, std::size_t a_BytesRead) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (m_bStopped) return;
            if (a_ErrorCode) {
                std::cerr << "Read of packet header failed, socket closed!" << std::endl;
                Close();
                return;
            } // if
            
            // Evaluate type field
            unsigned char l_Type = m_ReadBuffer[0];
            switch (l_Type & 0xF0) {
            case 0x00: {
                // Is data packet
                m_IncomingPacket = PacketData::CreateDeserializedPacket(l_Type);
                break;
            }
            case 0x10: {
                // Is control packet
                m_IncomingPacket = PacketCtrl::CreateDeserializedPacket(l_Type);
                break;
            }
            default:
                std::cerr << "Unknown content field in received packet header, socket closed!" << std::endl;
                Close();
                return;
            } // switch
            
            ReadRemainingBytes();
        });
    }
    
    void ReadRemainingBytes() {
        auto self(shared_from_this());
        if (m_bStopped) return;
        assert(m_IncomingPacket);
        size_t l_BytesNeeded = m_IncomingPacket->BytesNeeded();
        if (l_BytesNeeded) { // TODO: check buffer size!
            // More bytes needed 
            boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, l_BytesNeeded),
                                    [this, self](boost::system::error_code a_ErrorCode, std::size_t a_BytesRead) {
                if (a_ErrorCode == boost::asio::error::operation_aborted) return;
                if (m_bStopped) return;
                if (!a_ErrorCode) {
                    if (m_IncomingPacket->BytesReceived(m_ReadBuffer, a_BytesRead)) {
                        ReadRemainingBytes();
                    } else {
                        std::cerr << "TCP packet error!" << std::endl;
                        Close();
                    } // else
                } else {
                    std::cerr << "TCP read error!" << std::endl;
                    Close();
                } // else
            });
        } else {
            // Reception completed, deliver packet
            auto l_PacketData = std::dynamic_pointer_cast<PacketData>(m_IncomingPacket);
            if (l_PacketData) {
                if (m_OnDataCallback) {
                    // Deliver the data packet but stall the receiver
                    m_bReceiving = m_OnDataCallback(l_PacketData);
                } // if
            } else {
                auto l_PacketCtrl = std::dynamic_pointer_cast<PacketCtrl>(m_IncomingPacket);
                if (l_PacketCtrl) {
                    bool l_bDeliver = true;
                    if (l_PacketCtrl->GetPacketType() == PacketCtrl::CTRL_TYPE_KEEP_ALIVE) {
                        // This is a keep alive packet, simply drop it.
                        l_bDeliver = false;
                    } // if
                    
                    if ((l_bDeliver) && (m_OnCtrlCallback)) {
                        m_OnCtrlCallback(*(l_PacketCtrl.get()));
                    } // if
                } else {
                    assert(false);
                } // else
            } // else

            // Depending on the reveicer state, trigger reception of the next packet
            m_IncomingPacket.reset();
            if (m_bReceiving) {
                ReadType();
            } // if
        } // else
    }

    void do_write() {
        auto self(shared_from_this());
        if (m_bStopped) return;
        m_bWriteInProgress = true;
        boost::asio::async_write(m_TCPSocket, boost::asio::buffer(m_SendQueue.front().first.data(), m_SendQueue.front().first.size()),
                                 [this, self](boost::system::error_code a_ErrorCode, std::size_t a_BytesSent) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (m_bStopped) return;
            if (!a_ErrorCode) {
                // If a callback was provided, call it now to demand for a subsequent packet
                if (m_SendQueue.front().second) {
                    m_SendQueue.front().second();
                } // if

                m_SendQueue.pop_front();
                if (!m_SendQueue.empty()) {
                    do_write();
                } else {
                    m_bWriteInProgress = false;
                    if (m_bShutdown) {
                        m_SEPState = SEPSTATE_SHUTDOWN;
                        m_TCPSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                        Close();
                    } // if
                } // else
            } else {
                std::cerr << "TCP write error!" << std::endl;
                Close();
            }
        });
    }

private:
    // All possible callbacks for a user of this class
    std::function<bool(std::shared_ptr<const PacketData> a_PacketData)> m_OnDataCallback;
    std::function<void(const PacketCtrl& a_PacketCtrl)> m_OnCtrlCallback;
    std::function<void()> m_OnClosedCallback;
    
    boost::asio::ip::tcp::socket &m_TCPSocket;
    std::shared_ptr<Packet> m_IncomingPacket;
    std::deque<std::pair<std::vector<unsigned char>, std::function<void()>>> m_SendQueue; // To be transmitted
    bool m_bWriteInProgress;
    enum { max_length = 65535 };
    unsigned char m_ReadBuffer[max_length];
    
    // State
    typedef enum {
        SEPSTATE_DISCONNECTED = 0,
        SEPSTATE_CONNECTED    = 1,
        SEPSTATE_SHUTDOWN     = 2
    } E_SEPSTATE;
    E_SEPSTATE m_SEPState;
    bool m_bShutdown;
    bool m_bStarted;
    bool m_bStopped;
    bool m_bReceiving;
    
    // Timer
    boost::asio::deadline_timer m_KeepAliveTimer;
};

#endif // PACKET_ENDPOINT_H
