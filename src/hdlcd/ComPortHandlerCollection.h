/**
 * \file ComPortHandlerCollection.h
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

#ifndef COMPORTHANDLER_COLLECTION_H
#define COMPORTHANDLER_COLLECTION_H

#include <memory>
#include <string>
#include <map>
#include <boost/asio.hpp>
class ClientHandler;
class ComPortHandler;

class ComPortHandlerCollection: public std::enable_shared_from_this<ComPortHandlerCollection> {
public:
    ComPortHandlerCollection(boost::asio::io_service& a_IOService);
    std::shared_ptr<std::shared_ptr<ComPortHandler>> GetComPortHandler(const std::string &a_ComPortName, std::shared_ptr<ClientHandler> a_ClientHandler);
    
    // To be called by a ComPortHandler
    void DeregisterComPortHandler(std::shared_ptr<ComPortHandler> a_ComPortHandler);

private:
    // Members
    boost::asio::io_service& m_IOService;
    std::map<std::string, std::weak_ptr<std::shared_ptr<ComPortHandler>>> m_ComPortHandlerMap;
};

#endif // COMPORTHANDLER_COLLECTION_H
