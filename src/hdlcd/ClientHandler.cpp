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
#include "ComPortHandlerCollection.h"
#include "ComPortHandler.h"

ClientHandler::ClientHandler(boost::asio::ip::tcp::socket a_TCPSocket): m_TCPSocket(std::move(a_TCPSocket)) {
    m_Registered = true;
    m_CurrentlySending = false;
    std::cout << "CTOR ClientHandler" << std::endl;
}

ClientHandler::~ClientHandler() {
    std::cout << "DTOR ClientHandler" << std::endl;
}

void ClientHandler::DeliverPayloadToClients(const std::vector<unsigned char> &a_Payload) {
    std::lock_guard<std::mutex> l_MutexLock(m_SendMutex);
    m_SendBufferList.push_back(a_Payload);
    if (m_CurrentlySending == false) {
        m_CurrentlySending = true;
        do_write();
    } // if
}

void ClientHandler::Start(std::shared_ptr<ComPortHandlerCollection> a_ComPortHandlerCollection) {
    m_ComPortHandlerStopper = a_ComPortHandlerCollection->GetComPortHandler("/dev/ttyUSB0", shared_from_this());
    m_ComPortHandler = (*m_ComPortHandlerStopper.get());
    do_read();
}

void ClientHandler::Stop() {
    if (m_Registered) {
        m_Registered = false;
        m_ComPortHandler.reset();
        std::cout << "TCP CLOSE" << std::endl;
        m_TCPSocket.shutdown(m_TCPSocket.shutdown_both);
    } // if
}

void ClientHandler::do_read() {
    auto self(shared_from_this());
    m_TCPSocket.async_read_some(boost::asio::buffer(data_, max_length),[this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::cout << "TCP read " << length << " Bytes" << std::endl;
            std::vector<unsigned char> l_Buffer(length);
            memcpy(&(l_Buffer[0]), data_, length);
            
            
            
            /*
            std::vector<unsigned char> l_Payload(17);
            l_Payload[ 0] = 0x00;
            l_Payload[ 1] = 0x00;
            l_Payload[ 2] = 0x40;
            l_Payload[ 3] = 0x01;
            l_Payload[ 4] = 0x3F;
            l_Payload[ 5] = 0xF7;
            l_Payload[ 6] = 0x00;
            l_Payload[ 7] = 0x00;
            l_Payload[ 8] = 0x10;
            l_Payload[ 9] = 0x00;
            l_Payload[10] = 0x04;
            l_Payload[11] = 0x06;
            l_Payload[12] = 0x02;
            l_Payload[13] = 0x00;
            l_Payload[14] = 0x80;
            l_Payload[15] = 0x02;
            l_Payload[16] = 0x01;
            m_ComPortHandler->DeliverPayloadToHDLC(std::move(l_Payload));
            */
            
            
            
            
            
            
            m_ComPortHandler->DeliverPayloadToHDLC(std::move(l_Buffer));
            do_read();
        } else {
            std::cout << "TCP READ ERROR:" << ec << std::endl;
            Stop();
        } // else
    });
} 

void ClientHandler::do_write() {
    // The SendMutex was already locked here!
    auto self(shared_from_this());
    m_TCPSocket.async_write_some(boost::asio::buffer(&(m_SendBufferList.front()[0]), m_SendBufferList.front().size()),[this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            std::lock_guard<std::mutex> l_MutexLock(m_SendMutex);
            if (m_SendBufferList.front().size() == length) {
            } else {
                std::cout << "TCP Partly written: " << length << " of " << m_SendBufferList.front().size() << " bytes" << std::endl;
                assert(false);
            } // else
            
            // More to write?
            m_SendBufferList.pop_front();
            if (m_SendBufferList.empty() == false) {
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
