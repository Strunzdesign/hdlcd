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
#include <map>
#include <deque>
#include <vector>
#include <algorithm>
#include <mutex>
#include <assert.h>
#include <boost/asio.hpp>
class ComPortHandler;
class ComPortHandlerCollection;
using boost::asio::ip::tcp;

class ClientHandler: public std::enable_shared_from_this<ClientHandler> {
public:
    ClientHandler(boost::asio::ip::tcp::socket a_TCPSocket);
    ~ClientHandler();
    void DeliverPayloadToClients(const std::vector<unsigned char> &a_Payload);
    
    void Start(std::shared_ptr<ComPortHandlerCollection> a_ComPortHandlerCollection);
    void Stop();
    
private:
    // Helpers
    void do_read();
    void do_write();

    // Members
    bool m_Registered;
    boost::asio::ip::tcp::socket m_TCPSocket;
    std::shared_ptr<std::shared_ptr<ComPortHandler>> m_ComPortHandlerStopper;
    std::shared_ptr<ComPortHandler> m_ComPortHandler;
    enum { max_length = 1024 };
    char data_[max_length];
    
    std::mutex m_SendMutex;
    std::deque<std::vector<unsigned char>> m_SendBufferList;
    bool m_CurrentlySending;
};

#endif // CLIENTHANDLER_H
