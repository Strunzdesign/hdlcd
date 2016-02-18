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
#include <iomanip>
#include "ComPortHandlerCollection.h"
#include "ComPortHandler.h"
#include "../libFrame/StreamFrame.h"

ClientHandler::ClientHandler(boost::asio::ip::tcp::socket a_TCPSocket): m_TCPSocket(std::move(a_TCPSocket)) {
    m_Registered = true;
    m_CurrentlySending = false;
}

ClientHandler::~ClientHandler() {
}

void ClientHandler::DeliverBufferToClient(E_HDLCBUFFER a_eHDLCBuffer, E_DIRECTION a_eDirection, const std::vector<unsigned char> &a_Payload, bool a_bValid) {
    // Check whether this buffer is of interest to this specific client
    bool l_bDeliver = (a_eHDLCBuffer == m_eHDLCBuffer);
    if ((m_eDirection != DIRECTION_BOTH) && (m_eDirection != a_eDirection)) {
        l_bDeliver = false;
    } // if

    if (l_bDeliver) {
        StreamFrame l_StreamFrame;
        l_StreamFrame.body_length(a_Payload.size());
        std::memcpy(l_StreamFrame.body(), a_Payload.data(), l_StreamFrame.body_length());
        l_StreamFrame.encode_header();
        m_StreamFrameQueue.push_back(l_StreamFrame);
        if (m_CurrentlySending == false) {
            m_CurrentlySending = true;
            do_write();
        } // if
    } // if
}

void ClientHandler::Start(std::shared_ptr<ComPortHandlerCollection> a_ComPortHandlerCollection) {
    m_ComPortHandlerCollection = a_ComPortHandlerCollection;
    do_readSessionHeader1();
}

void ClientHandler::Stop() {
    if (m_Registered) {
        m_Registered = false;
        m_ComPortHandler.reset();
        std::cout << "TCP CLOSE" << std::endl;
        //m_TCPSocket.shutdown(m_TCPSocket.shutdown_both);
        m_TCPSocket.close();
    } // if
}

void ClientHandler::do_readSessionHeader1() {
    auto self(shared_from_this());
    m_TCPSocket.async_read_some(boost::asio::buffer(data_, 3),[this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            if ((data_[1] & 0x0F) == 0x01) {
                m_eDirection = DIRECTION_RCVD;
            } else if ((data_[1] & 0x0F) == 0x02) {
                m_eDirection = DIRECTION_SENT;
            } else if ((data_[1] & 0x0F) == 0x03) {
                m_eDirection = DIRECTION_BOTH;
            } // else if
            
            if ((data_[1] & 0xF0) == 0x00) {
                m_eHDLCBuffer = HDLCBUFFER_PAYLOAD;
                m_eDirection = DIRECTION_RCVD; // override
            } else if ((data_[1] & 0xF0) == 0x30) {
                m_eHDLCBuffer = HDLCBUFFER_RAW;
            } else if ((data_[1] & 0xF0) == 0x40) {
                m_eHDLCBuffer = HDLCBUFFER_DISSECTED;
            } // elseif
            
            // TODO: missing error management
            
            do_readSessionHeader2(data_[2]);
        } else {
            std::cout << "TCP READ ERROR HEADER1:" << ec << std::endl;
            Stop();
        } // else
    });
}

void ClientHandler::do_readSessionHeader2(unsigned char a_BytesUSB) {
    auto self(shared_from_this());
    m_TCPSocket.async_read_some(boost::asio::buffer(data_, max_length),[this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            // Now we know the USB port
            std::string l_UsbPortString;
            l_UsbPortString.append(data_, length);
            m_ComPortHandlerStopper = m_ComPortHandlerCollection->GetComPortHandler(l_UsbPortString, shared_from_this());
            m_ComPortHandler = (*m_ComPortHandlerStopper.get());
            do_read();
        } else {
            std::cout << "TCP READ ERROR:" << ec << std::endl;
            Stop();
        } // else
    });
}

void ClientHandler::do_read() {
    auto self(shared_from_this());
    m_TCPSocket.async_read_some(boost::asio::buffer(data_, max_length),[this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::vector<unsigned char> l_Buffer(length - 2);
            memcpy(&(l_Buffer[0]), &(data_[2]), length - 2);	    
            m_ComPortHandler->DeliverPayloadToHDLC(std::move(l_Buffer));
            do_read();
        } else {
            std::cout << "TCP READ ERROR:" << ec << std::endl;
            Stop();
        } // else
    });
}

void ClientHandler::do_write() {
    auto self(shared_from_this());
    boost::asio::async_write(m_TCPSocket, boost::asio::buffer(m_StreamFrameQueue.front().data(), m_StreamFrameQueue.front().length()),
                                 [this](boost::system::error_code ec, std::size_t /*length*/) {
        if (!ec) {
            // More to write?
            m_StreamFrameQueue.pop_front();
            if (!m_StreamFrameQueue.empty()) {
                do_write();
            } else {
                m_CurrentlySending = false;
            } // else
        } else {
            std::cout << "TCP WRITE ERROR:" << ec << std::endl;
            Stop();
        } // else
    });
}
