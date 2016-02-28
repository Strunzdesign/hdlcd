/**
 * \file ClientAcceptor.h
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

#ifndef CLIENT_ACCEPTOR_H
#define CLIENT_ACCEPTOR_H

#include <boost/asio.hpp>
#include "ClientHandler.h"
#include "../SerialPort/SerialPortHandlerCollection.h"
using boost::asio::ip::tcp;

class ClientAcceptor {
public:
ClientAcceptor(boost::asio::io_service& io_service, short port, std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection): m_SerialPortHandlerCollection(a_SerialPortHandlerCollection), m_TCPAcceptor(io_service, tcp::endpoint(tcp::v4(), port)), m_TCPSocket(io_service) {
    do_accept();
}

private:
    void do_accept() {
        m_TCPAcceptor.async_accept(m_TCPSocket, [this](boost::system::error_code a_ErrorCode) {
            if (!a_ErrorCode) {
                // ClientHandler objects are not stored explicetly. They keep themselves alive via shared pointers.
                std::make_shared<ClientHandler>(std::move(m_TCPSocket))->Start(m_SerialPortHandlerCollection);
            } // if

            do_accept();
        });
    }

    // Members
    std::shared_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection;
    tcp::acceptor m_TCPAcceptor;
    tcp::socket m_TCPSocket;
};

#endif // CLIENT_ACCEPTOR_H
