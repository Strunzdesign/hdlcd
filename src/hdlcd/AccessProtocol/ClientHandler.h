/**
 * \file ClientHandler.h
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

#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <iostream>
#include <memory>
#include <string>
#include <deque>
#include <vector>
#include <boost/asio.hpp>
#include "../SerialPort/SerialPortLockGuard.h"
#include "../HDLC/HDLCBuffer.h"
#include "../../shared/PacketEndpoint.h"
class SerialPortHandler;
class SerialPortHandlerCollection;

class ClientHandler: public std::enable_shared_from_this<ClientHandler> {
public:
    ClientHandler(boost::asio::ip::tcp::socket a_TCPSocket);
    ~ClientHandler();

    void DeliverBufferToClient(E_HDLCBUFFER a_eHDLCBuffer, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bValid, bool a_bWasSent);
    void UpdateSerialPortState(size_t a_LockHolders);
    
    void Start(std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection);
    void Stop();
    
private:
    // Helpers
    void ReadSessionHeader1();
    void ReadSessionHeader2(unsigned char a_BytesUSB);
    
    // Callbacks
    void OnDataReceived(const PacketData& a_PacketData);
    void OnCtrlReceived(const PacketCtrl& a_PacketCtrl);
    void OnClosed();

    // Members
    PacketEndpoint m_PacketEndpoint;
    
    bool m_Registered;
    boost::asio::ip::tcp::socket m_TCPSocket;
    std::shared_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection;
    std::shared_ptr<std::shared_ptr<SerialPortHandler>> m_SerialPortHandlerStopper;
    std::shared_ptr<SerialPortHandler> m_SerialPortHandler;
    enum { max_length = 256 };
    unsigned char m_ReadBuffer[max_length];

    SerialPortLockGuard m_SerialPortLockGuard;

    // SAP specification
    E_HDLCBUFFER m_eHDLCBuffer;
    bool m_bDeliverSent;
    bool m_bDeliverRcvd;
    bool m_bDeliverInvalidData;
};

#endif // CLIENTHANDLER_H
