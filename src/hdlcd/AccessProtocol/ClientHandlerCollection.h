/**
 * \file ClientHandlerCollection.h
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

#ifndef CLIENT_HANDLER_COLLECTION_H
#define CLIENT_HANDLER_COLLECTION_H

#include <memory>
#include <list>
class ClientHandler;

class ClientHandlerCollection {
public:
    void RegisterClientHandler(std::shared_ptr<ClientHandler> a_ClientHandler);
    void DeregisterClientHandler(std::shared_ptr<ClientHandler> a_ClientHandler);

private:
    // Members
    std::list<std::shared_ptr<ClientHandler>> m_ClientHandlerList;
};

#endif // CLIENT_HANDLER_COLLECTION_H
