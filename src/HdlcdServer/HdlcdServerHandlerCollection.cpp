/**
 * \file HdlcdServerHandlerCollection.cpp
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

#include "HdlcdServerHandlerCollection.h"
#include "HdlcdServerHandler.h"
#include <assert.h>
using boost::asio::ip::tcp;

HdlcdServerHandlerCollection::HdlcdServerHandlerCollection(boost::asio::io_service& a_IOService, std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection, uint16_t a_TcpPortNbr):
    m_IOService(a_IOService), m_SerialPortHandlerCollection(a_SerialPortHandlerCollection), m_TcpAcceptor(a_IOService, tcp::endpoint(tcp::v4(), a_TcpPortNbr)), m_TcpSocket(a_IOService) {
    // Checks
    assert(m_SerialPortHandlerCollection);
    
    // Trigger activity
    DoAccept();
}

void HdlcdServerHandlerCollection::Shutdown() {
    // Stop accepting subsequent TCP connections
    m_TcpAcceptor.close();

    // Release all client handler objects. They deregister themselves.
    while (!m_HdlcdServerHandlerList.empty()) {
        (*m_HdlcdServerHandlerList.begin())->Stop();
    } // while
    
    // Drop all shared pointers
    assert(m_HdlcdServerHandlerList.empty());
    m_SerialPortHandlerCollection.reset();
}

void HdlcdServerHandlerCollection::RegisterHdlcdServerHandler(std::shared_ptr<HdlcdServerHandler> a_HdlcdServerHandler) {
    m_HdlcdServerHandlerList.emplace_back(std::move(a_HdlcdServerHandler));
}

void HdlcdServerHandlerCollection::DeregisterHdlcdServerHandler(std::shared_ptr<HdlcdServerHandler> a_HdlcdServerHandler) {
    m_HdlcdServerHandlerList.remove(a_HdlcdServerHandler);
}

void HdlcdServerHandlerCollection::DoAccept() {
    m_TcpAcceptor.async_accept(m_TcpSocket, [this](boost::system::error_code a_ErrorCode) {
        if (!a_ErrorCode) {
            // Create a HDLCd server handler object and start it. It registers itself to the HDLCd server handler collection
            auto l_HdlcdServerHandler = std::make_shared<HdlcdServerHandler>(m_IOService, shared_from_this(), m_TcpSocket);
            l_HdlcdServerHandler->Start(m_SerialPortHandlerCollection);
        } // if

        // Wait for subsequent TCP connections
        DoAccept();
    }); // async_accept
}
