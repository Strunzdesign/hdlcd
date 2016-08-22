/**
 * \file SerialPortHandlerCollection.cpp
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

#include "SerialPortHandlerCollection.h"
#include "SerialPortHandler.h"
#include "../AccessProtocol/ClientHandler.h"

SerialPortHandlerCollection::SerialPortHandlerCollection(boost::asio::io_service& a_IOService): m_IOService(a_IOService) {
}

std::shared_ptr<std::shared_ptr<SerialPortHandler>> SerialPortHandlerCollection::GetSerialPortHandler(const std::string &a_SerialPortName, std::shared_ptr<ClientHandler> a_ClientHandler) {
    std::shared_ptr<std::shared_ptr<SerialPortHandler>> l_SerialPortHandler;
    bool l_HasToBeStarted = false;
    {
        auto& l_SerialPortHandlerWeak(m_SerialPortHandlerMap[a_SerialPortName]);
        l_SerialPortHandler = l_SerialPortHandlerWeak.lock();
        if (l_SerialPortHandler == NULL) {
            auto l_NewSerialPortHandler = std::make_shared<SerialPortHandler>(a_SerialPortName, shared_from_this(), m_IOService);
            std::shared_ptr<std::shared_ptr<SerialPortHandler>> l_NewSerialPortHandlerStopper(new std::shared_ptr<SerialPortHandler>(l_NewSerialPortHandler), [=](std::shared_ptr<SerialPortHandler>* todelete){ (*todelete)->Stop(); delete(todelete); });
            l_SerialPortHandler = l_NewSerialPortHandlerStopper;
            l_SerialPortHandlerWeak = l_SerialPortHandler;
            l_HasToBeStarted = true;
        } // if
    }
    
    l_SerialPortHandler.get()->get()->AddClientHandler(a_ClientHandler);
    if (l_HasToBeStarted) {
        if (l_SerialPortHandler.get()->get()->Start() == false) {
		    l_SerialPortHandler.reset();
		} // if
    } // if

    return l_SerialPortHandler;
}

void SerialPortHandlerCollection::DeregisterSerialPortHandler(std::shared_ptr<SerialPortHandler> a_SerialPortHandler) {
    assert(a_SerialPortHandler);
    for (auto it = m_SerialPortHandlerMap.begin(); it != m_SerialPortHandlerMap.end(); ++it) {
        if (auto cph = it->second.lock()) {
            if (*(cph.get()) == a_SerialPortHandler) {
                m_SerialPortHandlerMap.erase(it);
                break;
            } // if
        } // if
    } // for
}
