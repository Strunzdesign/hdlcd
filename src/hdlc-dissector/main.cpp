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
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <assert.h>
#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/asio.hpp>
#include "StreamFrame.h"
#include "StreamEndpoint.h"
using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        std::cerr << "HDLC dissector\n";
        if (argc != 4) {
            std::cerr << "Usage: hdlc-dissector <host> <port> <usb-device>\n";
            return 1;
        } // if

        boost::asio::io_service io_service;
        boost::asio::signal_set signals_(io_service);
        signals_.add(SIGINT);
        signals_.add(SIGTERM);
        signals_.async_wait([&io_service](boost::system::error_code errorCode, int signalNumber){io_service.stop();});
        
        // Resolve destination
        tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
        StreamEndpoint l_StreamEndpoint(io_service, endpoint_iterator, argv[3]);
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    std::cout << "FINISHED!" << std::endl;

    return 0;
}

