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
#include <boost/asio.hpp>
#include "../shared/PacketData.h"
#include "../shared/PacketCtrl.h"

class PacketEndpoint {
public:
    PacketEndpoint(boost::asio::ip::tcp::socket& a_TCPSocket): m_TCPSocket(a_TCPSocket) {
        m_SEPState = SEPSTATE_DISCONNECTED;
        m_bWriteInProgress = false;
        m_bShutdown = false;
    }
            
    // Callback methods
    void SetOnDataCallback(std::function<void(const PacketData& a_PacketData)> a_OnDataCallback) {
        m_OnDataCallback = a_OnDataCallback;
    }
    
    void SetOnCtrlCallback(std::function<void(const PacketCtrl& a_PacketCtrl)> a_OnCtrlCallback) {
        m_OnCtrlCallback = a_OnCtrlCallback;
    }
    
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }
    
    void Send(const Packet* a_Packet) {
        assert(a_Packet != NULL);
        if (m_SEPState == SEPSTATE_SHUTDOWN) {
            return;
        } // if

        m_SendQueue.emplace_back(std::move(a_Packet->Serialize()));
        bool write_in_progress = !m_SendQueue.empty();
        if ((!m_bWriteInProgress) && (!m_SendQueue.empty()) && (m_SEPState == SEPSTATE_CONNECTED)) {
            do_write();
        } // if
    }
    
    void Start() {
        assert(m_SEPState == SEPSTATE_DISCONNECTED);
        assert(m_bWriteInProgress == false);
        m_SEPState = SEPSTATE_CONNECTED;
        ReadType();
        if (!m_SendQueue.empty()) {
            do_write();
        } // if
    }

    void Shutdown() {
        m_bShutdown = true;
    }

    void close() {
        m_TCPSocket.close();
        if (m_OnClosedCallback) {
            m_OnClosedCallback();
        } // if
    }

private:
    void ReadType() {
        assert(m_IncomingPacket == false);
        boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, 1),
                                [this](boost::system::error_code a_ErrorCode, std::size_t a_BytesRead) {
            if (a_ErrorCode) {
                std::cerr << "Read of packet header failed, socket closed!" << std::endl;
                close();
                return;
            } // if
            
            // Evaluate type field
            unsigned char l_Type = m_ReadBuffer[0];
            switch (l_Type & 0xF0) {
            case 0x00: {
                // Is data packet
                m_IncomingPacket = std::make_shared<PacketData>(l_Type);
                break;
            }
            case 0x10: {
                // Is control packet
                m_IncomingPacket = std::make_shared<PacketCtrl>(l_Type);
                break;
            }
            default:
                std::cerr << "Unknown content field in received packet header, socket closed!" << std::endl;
                close();
                return;
            } // switch
            
            ReadRemainingBytes();
        });
    }
    
    void ReadRemainingBytes() {
        assert(m_IncomingPacket);
        size_t l_BytesNeeded = m_IncomingPacket->BytesNeeded();
        if (l_BytesNeeded) { // TODO: check buffer size!
            // More bytes needed 
            boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, l_BytesNeeded),
                                [this](boost::system::error_code a_ErrorCode, std::size_t a_BytesRead) {
                if (!a_ErrorCode) {
                    if (m_IncomingPacket->BytesReceived(m_ReadBuffer, a_BytesRead)) {
                        ReadRemainingBytes();
                    } else {
                        std::cerr << "TCP packet error!" << std::endl;
                        close();
                    } // else
                } else {
                    std::cerr << "TCP read error!" << std::endl;
                    close();
                } // else
            });
        } else {
            // Reception completed, deliver packet
            if (true) { // data packet
                if (m_OnDataCallback) {
                    m_OnDataCallback(*dynamic_cast<PacketData*>(m_IncomingPacket.get()));
                } // if
            } else {
                if (m_OnCtrlCallback) {
                    m_OnCtrlCallback(*dynamic_cast<PacketCtrl*>(m_IncomingPacket.get()));
                } // if
            } // else

            m_IncomingPacket.reset();
            ReadType();
        } // else
    }

    void do_write() {
        m_bWriteInProgress = true;
        boost::asio::async_write(m_TCPSocket, boost::asio::buffer(m_SendQueue.front().data(), m_SendQueue.front().size()),
                                 [this](boost::system::error_code a_ErrorCode, std::size_t a_BytesSent) {
            if (!a_ErrorCode) {
                m_SendQueue.pop_front();
                if (!m_SendQueue.empty()) {
                    do_write();
                } else {
                    m_bWriteInProgress = false;
                    if (m_bShutdown) {
                        m_SEPState = SEPSTATE_SHUTDOWN;
                        m_TCPSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                        close();
                    } // if
                } // else
            } else {
                std::cerr << "TCP write error!" << std::endl;
                close();
            }
        });
    }

private:
    // All possible callbacks for a user of this class
    std::function<void(const PacketData& a_PacketData)> m_OnDataCallback;
    std::function<void(const PacketCtrl& a_PacketCtrl)> m_OnCtrlCallback;
    std::function<void()> m_OnClosedCallback;
    
    boost::asio::ip::tcp::socket &m_TCPSocket;
    std::shared_ptr<Packet> m_IncomingPacket;
    std::deque<std::vector<unsigned char>> m_SendQueue; // To be transmitted
    bool m_bWriteInProgress;
    enum { max_length = 1024 };
    unsigned char m_ReadBuffer[max_length];
    
    // State
    typedef enum {
        SEPSTATE_DISCONNECTED = 0,
        SEPSTATE_CONNECTED    = 1,
        SEPSTATE_SHUTDOWN     = 2
    } E_SEPSTATE;
    E_SEPSTATE m_SEPState;
    bool m_bShutdown;
};

#endif // PACKET_ENDPOINT_H
