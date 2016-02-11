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
#include <thread>
#include <bitset>    // for std::bitset
#include <boost/asio.hpp>
#include "StreamFrame.h"
#include "StreamEndpoint.h"
using boost::asio::ip::tcp;

int main(int argc, char* argv[]) {
    try {
        std::cerr << "HDLC payload exchanger (hexdumps via STDIO)\n";
        if (argc != 4) {
            std::cerr << "Usage: hdlc-hexchange <host> <port> <usb-device>\n";
            return 1;
        } // if

        boost::asio::io_service io_service;
        tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });
        StreamEndpoint l_StreamEndpoint(io_service, endpoint_iterator, argv[3]);

        std::thread t([&io_service](){ io_service.run(); });

        size_t l_HexLineLength = ((StreamFrame::E_MAX_BODY_LENGTH * 3) + 1);
        char l_HexLine[l_HexLineLength];
        while (std::cin.getline(l_HexLine, l_HexLineLength)) {
            std::istringstream stream_in(l_HexLine);
            stream_in >> std::hex;
            std::vector<std::bitset<8>> memory;
            memory.reserve(65536);
            memory.insert(memory.end(),std::istream_iterator<unsigned int>(stream_in), {});
            
            // Put everything into the stream frame
            StreamFrame l_StreamFrame;
            l_StreamFrame.body_length(memory.size());
            std::memcpy(l_StreamFrame.body(), &memory[0], memory.size());
            l_StreamFrame.encode_header();
            l_StreamEndpoint.write(l_StreamFrame);
        } // while

        l_StreamEndpoint.close();
        t.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    
    std::cout << "FINISHED!" << std::endl;

    return 0;
}

