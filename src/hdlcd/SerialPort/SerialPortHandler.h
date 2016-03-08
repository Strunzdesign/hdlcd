/**
 * \file SerialPortHandler.h
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

#ifndef SERIAL_PORT_HANDLER_H
#define SERIAL_PORT_HANDLER_H

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <boost/asio.hpp>
#include "SerialPortLock.h"
#include "BaudRate.h"
#include "HDLC/HDLCBuffer.h"
class SerialPortHandlerCollection;
class ClientHandler;
class ProtocolState;

class SerialPortHandler: public std::enable_shared_from_this<SerialPortHandler> {
public:
    // CTOR and DTOR
    SerialPortHandler(const std::string &a_SerialPortName, std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection, boost::asio::io_service& a_IOService);
    ~SerialPortHandler();
    
    void AddClientHandler(std::shared_ptr<ClientHandler> a_ClientHandler);
    void DeliverPayloadToHDLC(const std::vector<unsigned char> &a_Payload, bool a_bReliable);
    void DeliverBufferToClients(E_HDLCBUFFER a_eHDLCBuffer, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bValid, bool a_bWasSent);
    
    bool Start();
    void Stop();
    void ChangeBaudRate();
    
    // Suspend / resume serial port
    void SuspendSerialPort();
    void ResumeSerialPort();
    void PropagateSerialPortState();
    
    // Do not use from external, only by the ProtocolState
    void TransmitHDLCFrame(const std::vector<unsigned char> &a_Payload);
    void QueryForPayload();

private:
    // Internal helpers
    void do_read();
    void do_write();
    void ForEachClient(std::function<void(std::shared_ptr<ClientHandler>)> a_Function);
    
    // Members
    bool m_Registered;
    boost::asio::serial_port m_SerialPort;
    boost::asio::io_service &m_IOService;
    std::shared_ptr<ProtocolState> m_ProtocolState;
    std::string m_SerialPortName;
    std::weak_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection;
    std::list<std::weak_ptr<ClientHandler>> m_ClientHandlerList;
    enum { max_length = 1024 };
    unsigned char m_ReadBuffer[max_length];
    
    std::vector<unsigned char> m_SendBuffer;
    size_t m_SendBufferOffset;
    SerialPortLock m_SerialPortLock;
    BaudRate m_BaudRate;
};

#endif // SERIAL_PORT_HANDLER_H
