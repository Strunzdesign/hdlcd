/**
 * \file StreamEndpoint.h
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

#ifndef STREAM_ENDPOINT_H
#define STREAM_ENDPOINT_H

#include <deque>
#include <iostream>
#include <boost/asio.hpp>
#include "../shared/StreamFrame.h"
#include "../shared/IBufferSink.h"

using boost::asio::ip::tcp;

typedef std::deque<StreamFrame> StreamFrameQueue;

class StreamEndpoint {
public:
    StreamEndpoint(boost::asio::io_service& a_IoService, tcp::resolver::iterator a_EndpointIterator, std::string a_ComPortString, IBufferSink* a_pBufferSink, unsigned char a_SAP):
        m_Socket(a_IoService),
        m_ComPortString(a_ComPortString),
        m_pBufferSink(a_pBufferSink),
        m_SAP(a_SAP) {
        m_SEPState = SEPSTATE_DISCONNECTED;
        m_bShutdown = false;
        do_connect(a_EndpointIterator);
    }
    
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }

    void write(const StreamFrame& a_StreamFrame) {
        if (m_SEPState == SEPSTATE_SHUTDOWN) {
            return;
        }

        bool write_in_progress = !m_StreamFrameQueue.empty();
        m_StreamFrameQueue.emplace_back(std::move(a_StreamFrame));
        if ((!write_in_progress) && (m_SEPState == SEPSTATE_CONNECTED)) {
            do_write();
        }
    }
    
    void Shutdown() {
        m_bShutdown = true;
    }

    void close() {
        m_Socket.close();
        if (m_OnClosedCallback) {
            m_OnClosedCallback();
        } // if
    }

private:
    void do_connect(tcp::resolver::iterator a_EndpointIterator) {
        boost::asio::async_connect(m_Socket, a_EndpointIterator,
                                   [this](boost::system::error_code ec, tcp::resolver::iterator) {
            if (!ec) {
                do_writeSessionHeader();
                m_SEPState = SEPSTATE_CONNECTED;
                do_read_header();
            } else {
                std::cerr << "TCP connect failed!" << std::endl;
                close();
            } // else
        });
    }

    void do_read_header() {
        boost::asio::async_read(m_Socket, boost::asio::buffer(m_StreamFrame.data(), StreamFrame::E_HEADER_LENGTH),
                                [this](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec && m_StreamFrame.DecodeHeader()) {
                do_read_body();
            } else {
                std::cerr << "Decode header failed, socket closed!" << std::endl;
                close();
            }
        });
    }

    void do_read_body() {
        boost::asio::async_read(m_Socket, boost::asio::buffer(m_StreamFrame.body(), m_StreamFrame.GetBodyLength()),
                                [this](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                std::vector<unsigned char> l_Buffer;
                l_Buffer.insert(l_Buffer.end(), m_StreamFrame.body(), m_StreamFrame.body() + m_StreamFrame.GetBodyLength());
                m_pBufferSink->BufferReceived((E_DIRECTION)m_StreamFrame.GetDirection(), l_Buffer);
                do_read_header();
            } else {
                std::cerr << "TCP read error!" << std::endl;
                close();
            } // else
        });
    }
    
    void do_writeSessionHeader() {
        // Prepare session header
        unsigned char l_SessionHeader[3 + m_ComPortString.size()];
        l_SessionHeader[0] = 0x00; // Version 0
        l_SessionHeader[1] = m_SAP;
        l_SessionHeader[2] = m_ComPortString.size();
        std::memcpy(&l_SessionHeader[3], m_ComPortString.data(), m_ComPortString.size());
        
        boost::asio::async_write(m_Socket, boost::asio::buffer(l_SessionHeader, 15),
                                 [this](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                if (!m_StreamFrameQueue.empty()) {
                    do_write();
                }
            } else {
                std::cerr << "TCP write error!" << std::endl;
                close();
            }
        });
    }

    void do_write() {
        boost::asio::async_write(m_Socket, boost::asio::buffer(m_StreamFrameQueue.front().data(), m_StreamFrameQueue.front().length()),
                                 [this](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                m_StreamFrameQueue.pop_front();
                if (!m_StreamFrameQueue.empty()) {
                    do_write();
                } else if (m_bShutdown) {
                    m_SEPState = SEPSTATE_SHUTDOWN;
                    m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                    close();
                } // else if
            } else {
                std::cerr << "TCP write error!" << std::endl;
                close();
            }
        });
    }

private:
    IBufferSink* m_pBufferSink;
    std::string m_ComPortString;
    unsigned char m_SAP;
    
    std::function<void()> m_OnClosedCallback;
    tcp::socket m_Socket;
    StreamFrame m_StreamFrame;
    StreamFrameQueue m_StreamFrameQueue;
    
    // State
    typedef enum {
        SEPSTATE_DISCONNECTED = 0,
        SEPSTATE_CONNECTED    = 1,
        SEPSTATE_SHUTDOWN     = 2
    } E_SEPSTATE;
    E_SEPSTATE m_SEPState;
    bool m_bShutdown;
};

#endif // STREAM_ENDPOINT_H
