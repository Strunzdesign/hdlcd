/**
 * \file HdlcdServerHandlerCollection.h
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

#ifndef HDLCD_SERVER_HANDLER_COLLECTION_H
#define HDLCD_SERVER_HANDLER_COLLECTION_H

#include <memory>
#include <list>
#include <boost/asio.hpp>
class SerialPortHandlerCollection;
class HdlcdServerHandler;

class HdlcdServerHandlerCollection: public std::enable_shared_from_this<HdlcdServerHandlerCollection> {
public:
    // CTOR and resetter
    HdlcdServerHandlerCollection(boost::asio::io_service& a_IOService, std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection, uint16_t a_TcpPortNbr);
    void Shutdown();
    
    // Self-registering and -deregistering of HDLCd server handler objects
    void RegisterHdlcdServerHandler  (std::shared_ptr<HdlcdServerHandler> a_HdlcdServerHandler);
    void DeregisterHdlcdServerHandler(std::shared_ptr<HdlcdServerHandler> a_HdlcdServerHandler);

private:
    // Internal helpers
    void DoAccept();

    // Members
    boost::asio::io_service& m_IOService;
    std::shared_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection;
    std::list<std::shared_ptr<HdlcdServerHandler>> m_HdlcdServerHandlerList;
    
    // Accept incoming TCP connections
    boost::asio::ip::tcp::tcp::acceptor m_TcpAcceptor; //!< The TCP listener
    boost::asio::ip::tcp::tcp::socket   m_TcpSocket; //!< One incoming TCP socket
};

#endif // HDLCD_SERVER_HANDLER_COLLECTION_H
