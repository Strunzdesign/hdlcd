/**
 * \file SerialPortHandler.cpp
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

#include "SerialPortHandler.h"
#include <boost/system/system_error.hpp>
#include "../AccessProtocol/ClientHandler.h"
#include "SerialPortHandlerCollection.h"
#include "HDLC/ProtocolState.h"

SerialPortHandler::SerialPortHandler(const std::string &a_SerialPortName, std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection, boost::asio::io_service &a_IOService): m_SerialPort(a_IOService), m_IOService(a_IOService) {
    m_Registered = true;
    m_SerialPortName = a_SerialPortName;
    m_SerialPortHandlerCollection = a_SerialPortHandlerCollection;
    m_SendBufferOffset = 0;
}

SerialPortHandler::~SerialPortHandler() {
    Stop();
}

void SerialPortHandler::AddClientHandler(std::shared_ptr<ClientHandler> a_ClientHandler) {
    m_ClientHandlerVector.push_back(a_ClientHandler);
}

void SerialPortHandler::SuspendSerialPort() {
    if (m_Registered == false) {
        return;
    } // if

    if (m_SerialPortLock.SuspendSerialPort()) {
        // The serial port is now suspended!
        m_ProtocolState->Stop();
        m_SerialPort.cancel();
        m_SerialPort.close();
    } // if
    
    PropagateSerialPortState();
}

void SerialPortHandler::ResumeSerialPort() {
    if (m_Registered == false) {
        return;
    } // if

    if (m_SerialPortLock.ResumeSerialPort()) {
        // The serial port is now resumed!
        m_SerialPort.open(m_SerialPortName);
        m_ProtocolState->Start();
        do_read();
    } // if
    
    PropagateSerialPortState();
}

void SerialPortHandler::PropagateSerialPortState() {
    ForEachClient([this](std::shared_ptr<ClientHandler> a_ClientHandler) {
        a_ClientHandler->UpdateSerialPortState(m_ProtocolState->IsAlive(), m_ProtocolState->IsFlowSuspended(), m_SerialPortLock.GetLockHolders());
    });
}

void SerialPortHandler::DeliverPayloadToHDLC(const std::vector<unsigned char> &a_Payload, bool a_bReliable) {
    m_ProtocolState->SendPayload(a_Payload, a_bReliable);
}

void SerialPortHandler::DeliverBufferToClients(E_HDLCBUFFER a_eHDLCBuffer, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bValid, bool a_bWasSent) {
    ForEachClient([a_eHDLCBuffer, a_Payload, a_bReliable, a_bValid, a_bWasSent](std::shared_ptr<ClientHandler> a_ClientHandler) {
        a_ClientHandler->DeliverBufferToClient(a_eHDLCBuffer, a_Payload, a_bReliable, a_bValid, a_bWasSent);
    });
}

bool SerialPortHandler::Start() {
    bool l_bResult = true;
    m_ProtocolState = std::make_shared<ProtocolState>(shared_from_this(), m_IOService);
    try {
        m_SerialPort.open(m_SerialPortName);
        m_SerialPort.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
        m_SerialPort.set_option(boost::asio::serial_port::character_size(boost::asio::serial_port::character_size(8)));
        m_SerialPort.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
        m_SerialPort.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none));
        ChangeBaudRate();
        
        // Start processing
        m_ProtocolState->Start();
        do_read();
    } catch (boost::system::system_error& error) {
        std::cerr << error.what() << std::endl;
        l_bResult = false;
        m_Registered = false;
        
        // TODO: ugly, code duplication. We must assure that cancel is not called!
        auto self(shared_from_this());
        m_ProtocolState->Shutdown();
        if (auto l_SerialPortHandlerCollection = m_SerialPortHandlerCollection.lock()) {
            l_SerialPortHandlerCollection->DeregisterSerialPortHandler(self);
        } // if
        
        for (auto it = m_ClientHandlerVector.begin(); it != m_ClientHandlerVector.end(); ++it) {
            if (auto l_ClientHandler = it->lock()) {
                l_ClientHandler->Stop();
            } // if
        } // for
    } // catch

    return l_bResult;
}

void SerialPortHandler::Stop() {
    if (m_Registered) {
        m_Registered = false;
        
        // Keep a copy here to keep this object alive!
        auto self(shared_from_this());
        m_SerialPort.cancel();
        m_SerialPort.close();
        m_ProtocolState->Shutdown();
        if (auto l_SerialPortHandlerCollection = m_SerialPortHandlerCollection.lock()) {
            l_SerialPortHandlerCollection->DeregisterSerialPortHandler(self);
        } // if
        
        for (auto it = m_ClientHandlerVector.begin(); it != m_ClientHandlerVector.end(); ++it) {
            if (auto l_ClientHandler = it->lock()) {
                l_ClientHandler->Stop();
            } // if
        } // for
    } // if
}

void SerialPortHandler::ChangeBaudRate() {
    if (m_Registered) {
        m_SerialPort.set_option(boost::asio::serial_port::baud_rate(m_BaudRate.GetNextBaudRate()));
    } // if
}

void SerialPortHandler::TransmitHDLCFrame(const std::vector<unsigned char> &a_Payload) {
    // Copy buffer holding the escaped HDLC frame for transmission via the serial interface
    assert(m_SendBufferOffset == 0);
    assert(m_SerialPortLock.GetSerialPortState() == false);
    m_SendBuffer = std::move(a_Payload);
    
    // Trigger transmission
    do_write();
}

void SerialPortHandler::QueryForPayload() {
    ForEachClient([](std::shared_ptr<ClientHandler> a_ClientHandler) {
        a_ClientHandler->QueryForPayload();
    });
}

void SerialPortHandler::do_read() {
    auto self(shared_from_this());
    m_SerialPort.async_read_some(boost::asio::buffer(m_ReadBuffer, max_length),[this, self](boost::system::error_code a_ErrorCode, std::size_t a_BytesRead) {
        if (a_ErrorCode == boost::asio::error::operation_aborted) return;
        if (!a_ErrorCode) {
            m_ProtocolState->AddReceivedRawBytes(m_ReadBuffer, a_BytesRead);
            if (m_SerialPortLock.GetSerialPortState() == false) {
                do_read();
            } // if
        } else {
            if (m_SerialPortLock.GetSerialPortState() == false) {
                std::cerr << "SERIAL READ ERROR:" << a_ErrorCode << std::endl;
                Stop();
            } // if
        } 
    });
}

void SerialPortHandler::do_write() {
    auto self(shared_from_this());
    m_SerialPort.async_write_some(boost::asio::buffer(&m_SendBuffer[m_SendBufferOffset], (m_SendBuffer.size() - m_SendBufferOffset)),[this, self](boost::system::error_code a_ErrorCode, std::size_t a_BytesRead) {
        if (a_ErrorCode == boost::asio::error::operation_aborted) return;
        if (!a_ErrorCode) {
            m_SendBufferOffset += a_BytesRead;
            if (m_SendBufferOffset == m_SendBuffer.size()) {
                // Indicate that we are ready to transmit the next HDLC frame
                m_SendBufferOffset = 0;
                if (m_SerialPortLock.GetSerialPortState() == false) {
                    m_ProtocolState->TriggerNextHDLCFrame();
                } // if
            } else {
                // Only a partial transmission. We are not done yet.
                if (m_SerialPortLock.GetSerialPortState() == false) {
                    do_write();
                } // if
            } // else
        } else {
            if (m_SerialPortLock.GetSerialPortState() == false) {
                std::cerr << "SERIAL WRITE ERROR:" << a_ErrorCode << std::endl;
                Stop();
            } // if
        } // else
    });
}

void SerialPortHandler::ForEachClient(std::function<void(std::shared_ptr<ClientHandler>)> a_Function) {
    for (auto cur = m_ClientHandlerVector.begin(); cur != m_ClientHandlerVector.end();) {
        auto next = cur;
        ++next;
        if (auto l_ClientHandler = cur->lock()) {
            a_Function(l_ClientHandler);
        } else {
            // Outdated entry
            m_ClientHandlerVector.erase(cur);
        } // else
        
        cur = next;
    } // for
}
