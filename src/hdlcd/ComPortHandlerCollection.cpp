/**
 * \file ComPortHandlerCollection.cpp
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

#include "ComPortHandlerCollection.h"
#include "ClientHandler.h"
#include "ComPortHandler.h"

ComPortHandlerCollection::ComPortHandlerCollection(boost::asio::io_service& a_IOService): m_IOService(a_IOService) {
}

std::shared_ptr<std::shared_ptr<ComPortHandler>> ComPortHandlerCollection::GetComPortHandler(const std::string &a_ComPortName, std::shared_ptr<ClientHandler> a_ClientHandler) {
    std::shared_ptr<std::shared_ptr<ComPortHandler>> l_ComPortHandler;
    bool l_HasToBeStarted = false;
    {
        auto& l_ComPortHandlerWeak(m_ComPortHandlerMap[a_ComPortName]);
        l_ComPortHandler = l_ComPortHandlerWeak.lock();
        if (l_ComPortHandler == NULL) {
            auto l_NewComPortHandler = std::make_shared<ComPortHandler>(a_ComPortName, shared_from_this(), m_IOService);
            std::shared_ptr<std::shared_ptr<ComPortHandler>> l_NewComPortHandlerStopper(new std::shared_ptr<ComPortHandler>(l_NewComPortHandler), [=](std::shared_ptr<ComPortHandler>* todelete){ (*todelete)->Stop(); delete(todelete); });
            l_ComPortHandler = std::move(l_NewComPortHandlerStopper);
            l_ComPortHandlerWeak = l_ComPortHandler;
            l_HasToBeStarted = true;
        } // if
    }
    
    l_ComPortHandler.get()->get()->AddClientHandler(a_ClientHandler);
    if (l_HasToBeStarted) {
        l_ComPortHandler.get()->get()->Start();
    } // if

    return std::move(l_ComPortHandler);
}

void ComPortHandlerCollection::DeregisterComPortHandler(std::shared_ptr<ComPortHandler> a_ComPortHandler) {
    assert(a_ComPortHandler);
    for (auto it = m_ComPortHandlerMap.begin(); it != m_ComPortHandlerMap.end(); ++it) {
        if (auto cph = it->second.lock()) {
            if (*(cph.get()) == a_ComPortHandler) {
                m_ComPortHandlerMap.erase(it);
                break;
            }
        }
    }
}
