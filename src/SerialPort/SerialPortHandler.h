/**
 * \file SerialPortHandler.h
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

#ifndef SERIAL_PORT_HANDLER_H
#define SERIAL_PORT_HANDLER_H

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <boost/asio.hpp>
#include "ISerialPortHandler.h"
#include "SerialPortLock.h"
#include "BaudRate.h"
class SerialPortHandlerCollection;
class HdlcdServerHandler;
class ProtocolState;

class SerialPortHandler: public ISerialPortHandler, public std::enable_shared_from_this<SerialPortHandler> {
public:
    // CTOR and DTOR
    SerialPortHandler(const std::string &a_SerialPortName, std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection, boost::asio::io_service& a_IOService);
    ~SerialPortHandler();
    
    void AddHdlcdServerHandler(std::shared_ptr<HdlcdServerHandler> a_HdlcdServerHandler);
    void DeliverPayloadToHDLC(const std::vector<unsigned char> &a_Payload, bool a_bReliable);
    
    bool Start();
    void Stop();
    
    // Suspend / resume serial port
    void SuspendSerialPort();
    void ResumeSerialPort();
    
    void PropagateSerialPortState();

private:
    // Called by a ProtocolState object
    bool RequiresBufferType(E_BUFFER_TYPE a_eBufferType) const;
    void DeliverBufferToClients(E_BUFFER_TYPE a_eBufferType, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bInvalid, bool a_bWasSent);
    bool OpenSerialPort();
    void ChangeBaudRate();
    void TransmitHDLCFrame(const std::vector<unsigned char> &a_Payload);
    void QueryForPayload(bool a_bQueryReliable, bool a_bQueryUnreliable);

    // Internal helpers
    void DoRead();
    void DoWrite();
    void ForEachHdlcdServerHandler(std::function<void(std::shared_ptr<HdlcdServerHandler>)> a_Function);
    
    // Members
    bool m_Registered;
    boost::asio::serial_port m_SerialPort;
    boost::asio::io_service &m_IOService;
    std::shared_ptr<ProtocolState> m_ProtocolState;
    std::string m_SerialPortName;
    std::weak_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection;
    std::list<std::weak_ptr<HdlcdServerHandler>> m_HdlcdServerHandlerList;
    enum { max_length = 1024 };
    unsigned char m_ReadBuffer[max_length];
    
    std::vector<unsigned char> m_SendBuffer;
    size_t m_SendBufferOffset;
    SerialPortLock m_SerialPortLock;
    BaudRate m_BaudRate;
    
    // Track all subscribed clients
    size_t m_BufferTypeSubscribers[BUFFER_TYPE_ARITHMETIC_ENDMARKER];
};

#endif // SERIAL_PORT_HANDLER_H
