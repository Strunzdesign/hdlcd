/**
 * \file main-hdlcd.cpp
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

#include "Config.h"
#include <iostream>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include "SerialPort/SerialPortHandlerCollection.h"
#include "AccessProtocol/ClientAcceptor.h"

int main(int argc, char **argv) {
    try {
        // Declare the supported options.
        boost::program_options::options_description l_Description("Allowed options");
        l_Description.add_options()
            ("help,h", "produce this help message")
            ("version,v", "show version information")
            ("port,p", boost::program_options::value<uint16_t>(), "the TCP port to accept clients on")
        ;

        // Parse the command line
        boost::program_options::variables_map l_VariablesMap;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, l_Description), l_VariablesMap);
        boost::program_options::notify(l_VariablesMap);
        if (l_VariablesMap.count("version")) {
            std::cerr << "HDLC daemon v" << HDLCD_VERSION_MAJOR << "." << HDLCD_VERSION_MINOR
                      << " built with hdlcd-devel v" << HDLCD_DEVEL_VERSION_MAJOR << "." << HDLCD_DEVEL_VERSION_MINOR << std::endl;
        } // if

        if (l_VariablesMap.count("help")) {
            std::cout << l_Description << std::endl;
            return 1;
        } // if
                        
        if (!l_VariablesMap.count("port")) {
            std::cout << "you have to specify the TCP listener port" << std::endl;
            return 1;
        } // if

        // Install signal handlers
        boost::asio::io_service l_IoService;
        boost::asio::signal_set l_Signals(l_IoService);
        l_Signals.add(SIGINT);
        l_Signals.add(SIGTERM);
        l_Signals.async_wait([&l_IoService](boost::system::error_code a_ErrorCode, int a_SignalNumber){ l_IoService.stop(); });
        
        // Create the HDLCd entity
        auto l_SerialPortHandlerCollection = std::make_shared<SerialPortHandlerCollection>(l_IoService);
        ClientAcceptor l_ClientAcceptor(l_IoService, l_VariablesMap["port"].as<uint16_t>(), l_SerialPortHandlerCollection);
        
        // Start event processing
        l_IoService.run();
    } catch (std::exception& a_ErrorCode) {
        std::cerr << "Exception: " << a_ErrorCode.what() << "\n";
        return 1;
    } // catch
    
    return 0;
}
