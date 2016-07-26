/**
 * \file      ClientAcceptor.h
 * \brief     This file contains the header declaration of class ClientAcceptor
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright GNU Public License version 3.
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
#include "ClientHandlerCollection.h"
#include "../SerialPort/SerialPortHandlerCollection.h"

using boost::asio::ip::tcp;

/*! \class ClientAcceptor
 *  \brief Class ClientAcceptor
 * 
 *  This class is responsible to accept incoming TCP connections. For each inbound TCP connection, a ClientHandler object is creating
 *  taking full responsibility of the connection. These TCP connections originate from clients that access the HDLCd and contain PDUs
 *  of the HDLCd access protocol.
 */
class ClientAcceptor {
public:
    /*! \brief The constructor of class ClientAcceptor
     * 
     *  Listener is started directly on instantiation (RAII)
     * 
     *  \param a_IOService the boost IOService object
     *  \param a_usPortNbr the TCP port number to wait for incoming TCP connections
     *  \param a_SerialPortHandlerCollection the collection helper class of serial port handlers responsible for talking to devices using the HDLC protocol 
     */
    ClientAcceptor(boost::asio::io_service& a_IOService, unsigned short a_usPortNbr, std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection): m_SerialPortHandlerCollection(a_SerialPortHandlerCollection), m_TCPAcceptor(a_IOService, tcp::endpoint(tcp::v4(), a_usPortNbr)), m_TCPSocket(a_IOService) {
        // Create the collection helper regarding accepted clients
        m_ClientHandlerCollection = std::make_shared<ClientHandlerCollection>();
        do_accept(); // start accepting TCP connections
    }

private:
    /*! \brief Internal callback to handle a single incoming TCP connection
     * 
     *  In this internal callback function, for each incoming TCP connection a ClientHandler object is created. This handler consumes the TCP connection and adds itself
     *  to the collection of client handlers.
     */
    void do_accept() {
        m_TCPAcceptor.async_accept(m_TCPSocket, [this](boost::system::error_code a_ErrorCode) {
            if (!a_ErrorCode) {
                // Create ClientHandler, store, and start it
                auto l_ClientHandler = std::make_shared<ClientHandler>(m_ClientHandlerCollection, std::move(m_TCPSocket));
                l_ClientHandler->Start(m_SerialPortHandlerCollection);
            } // if

            // Wait for subsequent TCP connections
            do_accept();
        });
    }

    // Members
    std::shared_ptr<ClientHandlerCollection> m_ClientHandlerCollection; //!< This object is responsible for holding all client handlers
    std::shared_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection; //!< This object is responsible for holding all serial port handlers
    tcp::acceptor m_TCPAcceptor; //!< The TCP listener
    tcp::socket m_TCPSocket; //!< One incoming TCP socket
};

#endif // CLIENT_ACCEPTOR_H
