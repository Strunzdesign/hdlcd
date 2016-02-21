/**
 * \file SerialPortHandlerCollection.h
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

#ifndef SERIAL_PORT_HANDLER_COLLECTION_H
#define SERIAL_PORT_HANDLER_COLLECTION_H

#include <memory>
#include <string>
#include <map>
#include <boost/asio.hpp>
class ClientHandler;
class SerialPortHandler;

class SerialPortHandlerCollection: public std::enable_shared_from_this<SerialPortHandlerCollection> {
public:
    SerialPortHandlerCollection(boost::asio::io_service& a_IOService);
    std::shared_ptr<std::shared_ptr<SerialPortHandler>> GetSerialPortHandler(const std::string &a_SerialPortName, std::shared_ptr<ClientHandler> a_ClientHandler);
    
    // To be called by a SerialPortHandler
    void DeregisterSerialPortHandler(std::shared_ptr<SerialPortHandler> a_SerialPortHandler);

private:
    // Members
    boost::asio::io_service& m_IOService;
    std::map<std::string, std::weak_ptr<std::shared_ptr<SerialPortHandler>>> m_SerialPortHandlerMap;
};

#endif // SERIAL_PORT_HANDLER_COLLECTION_H
