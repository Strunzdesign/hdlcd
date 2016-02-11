/**
 * \file ComPortHandler.cpp
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

#include "ComPortHandler.h"
#include "ComPortHandlerCollection.h"
#include "ClientHandler.h"
#include "HDLC/ProtocolState.h"
#include <boost/system/system_error.hpp>

ComPortHandler::ComPortHandler(const std::string &a_ComPortName, std::shared_ptr<ComPortHandlerCollection> a_ComPortHandlerCollection, boost::asio::io_service &a_IOService): m_SerialPort(a_IOService), m_IOService(a_IOService) {
    std::cout << "CTOR ComPortHandler" << std::endl;
    m_CurrentlySending = false;
    m_Registered = true;
    m_ComPortName = a_ComPortName;
    m_ComPortHandlerCollection = a_ComPortHandlerCollection;
}

ComPortHandler::~ComPortHandler() {
    std::cout << "DTOR ComPortHandler" << std::endl;
    Stop();
}

void ComPortHandler::AddClientHandler(std::shared_ptr<ClientHandler> a_ClientHandler) {
    m_ClientHandlerVector.push_back(a_ClientHandler);
}

void ComPortHandler::DeliverPayloadToHDLC(const std::vector<unsigned char> &a_Payload) {
    m_ProtocolState->SendPayload(a_Payload);
}

void ComPortHandler::DeliverPayloadToClients(const std::vector<unsigned char> &a_Payload) {
    for (auto it = m_ClientHandlerVector.begin(); it != m_ClientHandlerVector.end(); ++it) {
        if (auto l_ClientHandler = it->lock()) {
            l_ClientHandler->DeliverPayloadToClients(a_Payload);
        } // if
        // TODO: REMOVE IF INVALID
    } // for
}

void ComPortHandler::Start() {
    try {
        m_ProtocolState = std::make_shared<ProtocolState>(shared_from_this(), m_IOService);
        m_ProtocolState->Start();

        m_SerialPort.open(m_ComPortName);
        m_SerialPort.set_option(boost::asio::serial_port::baud_rate(115200));
        m_SerialPort.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
        m_SerialPort.set_option(boost::asio::serial_port::character_size(boost::asio::serial_port::character_size(8)));
        m_SerialPort.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
        m_SerialPort.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none));
        
        // Start processing
        do_read();
        /*
        serialPort.close();
        */
    } catch (boost::system::system_error& error) {
        std::cout << error.what() << std::endl;
        Stop();
    } // catch
}

void ComPortHandler::Stop() {
    if (m_Registered) {
        m_Registered = false;
        
        // Keep a copy here to keep this object alive!
        auto self(shared_from_this());
        std::cout << "SERIAL CLOSE" << std::endl;
        m_SerialPort.close();
        
        m_ProtocolState->Stop();
        
        if (auto l_ComPortHandlerCollection = m_ComPortHandlerCollection.lock()) {
            l_ComPortHandlerCollection->DeregisterComPortHandler(self);
        } // if
        
        for (auto it = m_ClientHandlerVector.begin(); it != m_ClientHandlerVector.end(); ++it) {
            if (auto l_ClientHandler = it->lock()) {
                l_ClientHandler->Stop();
            } // if
        } // for
    } // if
}

void ComPortHandler::DeliverHDLCFrame(const std::vector<unsigned char> &a_Payload) {
    // DUMP
    for (size_t l_Index = 0; l_Index < a_Payload.size(); ++l_Index) {
        std::cout << int(a_Payload[l_Index]) << " ";
    } // for
    
    std::cout << std::endl;

    m_SendBufferList.push_back(a_Payload);
    if (m_CurrentlySending == false) {
        m_CurrentlySending = true;
        do_write();
    } // if
}

void ComPortHandler::do_read() {
    auto self(shared_from_this());
    m_SerialPort.async_read_some(boost::asio::buffer(data_, max_length),[this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            m_ProtocolState->AddReceivedRawBytes(data_, length);
            do_read();
        } else {
            std::cout << "SERIAL READ ERROR:" << ec << std::endl;
            Stop();
        } 
    });
}

void ComPortHandler::do_write() {
    auto self(shared_from_this());
    m_SerialPort.async_write_some(boost::asio::buffer(&(m_SendBufferList.front()[0]), m_SendBufferList.front().size()),[this, self](boost::system::error_code ec, std::size_t length) {
        bool l_bQueryForSubsequentFrames = false;
        if (!ec) {
            if (m_SendBufferList.front().size() == length) {
                std::cout << "SERIAL Completely written: " << length << " bytes" << std::endl;
            } else {
                std::cout << "SERIAL Partly written: " << length << " of " << m_SendBufferList.front().size() << " bytes" << std::endl;
                assert(false);
            } // else
            
            // More to write?
            m_SendBufferList.pop_front();
            if (m_SendBufferList.empty() == false) {
                do_write();
            } else {
                // SendBuffer is empty. Actively query for more.
                m_CurrentlySending = false;
                l_bQueryForSubsequentFrames = true;
            } // else
        } else {
            std::cout << "SERIAL WRITE ERROR:" << ec << std::endl;
            Stop();
        } // else
        
        if (l_bQueryForSubsequentFrames) {
            m_ProtocolState->SendQueueIsEmpty();
        } // if
    });
}
