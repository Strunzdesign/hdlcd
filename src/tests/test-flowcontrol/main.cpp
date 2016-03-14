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
#include <vector>
#include <boost/asio.hpp>
#include "../../shared/AccessClient.h"

class DataSource {
public:
    DataSource(boost::asio::io_service& a_IoService, AccessClient& a_AccessClient): m_Timer(a_IoService), m_AccessClient(a_AccessClient), m_ucSeqNr(0) {
        StartTimer();
    }
    
private:
    // Helpers
    bool SendNextPacket() {
        // Create packet
        std::vector<unsigned char> l_Buffer = {0x00, 0x00, 0x40, 0x01, 0x3F, 0xF7, 0x00, 0x00, 0x10, 0x00, 0x04, 0x06, 0x02, 0x00, 0x80, 0x02, 0x01, m_ucSeqNr};
        if (m_AccessClient.Send(std::move(PacketData::CreatePacket(l_Buffer, true)))) {
            if ((++m_ucSeqNr) == 100) {
                m_ucSeqNr = 0;
                std::cout << "100 Packets written" << std::endl;
            } // if
            
            return true;
        } // if
        
        return false;
    }
    
    void StartTimer() {
        m_Timer.expires_from_now(boost::posix_time::milliseconds(2));
        m_Timer.async_wait([this](const boost::system::error_code& a_ErrorCode) {
            if (!a_ErrorCode) {
                while (SendNextPacket());
                StartTimer();
            } // if
        });
    }
    
    // Members
    AccessClient& m_AccessClient;
    unsigned char m_ucSeqNr;
    boost::asio::deadline_timer m_Timer;
};


int main(int argc, char* argv[]) {
    try {
        std::cerr << "HDLC flowcontrol tester\n";
        if (argc != 4) {
            std::cerr << "Usage: test-flowcontrol <host> <port> <usb-device>\n";
            return 1;
        } // if

        // Install signal handlers
        boost::asio::io_service io_service;
        boost::asio::signal_set signals_(io_service);
        signals_.add(SIGINT);
        signals_.add(SIGTERM);
        signals_.async_wait([&io_service](boost::system::error_code errorCode, int signalNumber){io_service.stop();});
        
        // Resolve destination
        boost::asio::ip::tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({ argv[1], argv[2] });

        // Prepare access protocol entity
        AccessClient l_AccessClient(io_service, endpoint_iterator, argv[3], 0x00);
        l_AccessClient.SetOnClosedCallback([&io_service](){io_service.stop();});

        // Prepare input
        DataSource l_DataSource(io_service, l_AccessClient);

        // Start event processing
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    } // catch
    
    return 0;
}

