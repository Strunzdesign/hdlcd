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
#include "../../shared/Direction.h"
#include "../../shared/StreamFrame.h"
class SerialPortHandler;
class SerialPortHandlerCollection;
using boost::asio::ip::tcp;

class ClientHandler: public std::enable_shared_from_this<ClientHandler> {
public:
    ClientHandler(boost::asio::ip::tcp::socket a_TCPSocket);
    ~ClientHandler();

    void DeliverBufferToClient(E_HDLCBUFFER a_eHDLCBuffer, E_DIRECTION a_eDirection, const std::vector<unsigned char> &a_Payload, bool a_bValid);
    void UpdateSerialPortState(size_t a_LockHolders);
    
    void Start(std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection);
    void Stop();
    
private:
    // Helpers
    void do_readSessionHeader1();
    void do_readSessionHeader2(unsigned char a_BytesUSB);
    void do_read_header();
    void do_read_body();
    void do_write();

    // Members
    bool m_Registered;
    boost::asio::ip::tcp::socket m_TCPSocket;
    std::shared_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection;
    std::shared_ptr<std::shared_ptr<SerialPortHandler>> m_SerialPortHandlerStopper;
    std::shared_ptr<SerialPortHandler> m_SerialPortHandler;
    enum { max_length = 1024 };
    char data_[max_length];
    
    StreamFrame m_StreamFrame;
    
    SerialPortLockGuard m_SerialPortLockGuard;
    std::deque<StreamFrame> m_StreamFrameQueue;
    bool m_CurrentlySending;
    
    // SAP specification
    E_HDLCBUFFER m_eHDLCBuffer;
    E_DIRECTION  m_eDirection;
};

#endif // CLIENTHANDLER_H
