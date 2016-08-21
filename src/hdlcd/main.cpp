/**
 * \file main.cpp
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

#include <iostream>
#include <boost/asio.hpp>
#include "SerialPort/SerialPortHandlerCollection.h"
#include "AccessProtocol/ClientAcceptor.h"

int main(int argc, char **argv) {
    std::cerr << "HDLC daemon\n";
    if (argc != 2) {
        std::cerr << "Usage: hdlcd <port>\n";
        return 1;
    } // if
        
    boost::asio::io_service io_service;
    boost::asio::signal_set signals_(io_service);
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
    signals_.async_wait([&io_service](boost::system::error_code a_ErrorCode, int a_SignalNumber){io_service.stop();});
    
    auto l_SerialPortHandlerCollection = std::make_shared<SerialPortHandlerCollection>(io_service);
    ClientAcceptor l_ClientAcceptor(io_service, atoi(argv[1]), l_SerialPortHandlerCollection);
    io_service.run(); 
    return 0;
}
