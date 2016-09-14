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
#include "SerialPortHandlerCollection.h"
#include "HdlcdServerHandlerCollection.h"

int main(int argc, char **argv) {
    try {
        // Declare the supported options.
        boost::program_options::options_description l_Description("Allowed options");
        l_Description.add_options()
            ("help,h",    "produce this help message")
            ("version,v", "show version information")
            ("port,p",    boost::program_options::value<uint16_t>(),
                          "the TCP port to accept clients on")
        ;

        // Parse the command line
        boost::program_options::variables_map l_VariablesMap;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, l_Description), l_VariablesMap);
        boost::program_options::notify(l_VariablesMap);
        if (l_VariablesMap.count("version")) {
            std::cerr << "HDLC daemon version " << HDLCD_VERSION_MAJOR << "." << HDLCD_VERSION_MINOR
                      << " built with hdlcd-devel version " << HDLCD_DEVEL_VERSION_MAJOR << "." << HDLCD_DEVEL_VERSION_MINOR << std::endl;
        } // if

        if (l_VariablesMap.count("help")) {
            std::cout << l_Description << std::endl;
            std::cout << "The HDLC Daemon is Copyright (C) 2016, and GNU GPL'd, by Florian Evers." << std::endl;
            std::cout << "Bug reports, feedback, admiration, abuse, etc, to: https://github.com/Strunzdesign/hdlcd" << std::endl;
            return 1;
        } // if
                        
        if (!l_VariablesMap.count("port")) {
            std::cout << "hdlcd: you have to specify the TCP listener port" << std::endl;
            std::cout << "hdlcd: Use --help for more information." << std::endl;
            return 1;
        } // if

        // Install signal handlers
        boost::asio::io_service l_IoService;
        boost::asio::signal_set l_Signals(l_IoService);
        l_Signals.add(SIGINT);
        l_Signals.add(SIGTERM);
        l_Signals.async_wait([&l_IoService](boost::system::error_code, int){ l_IoService.stop(); });
        
        // Create and initialize components
        auto l_SerialPortHandlerCollection  = std::make_shared<SerialPortHandlerCollection> (l_IoService);
        auto l_HdlcdServerHandlerCollection = std::make_shared<HdlcdServerHandlerCollection>(l_IoService, l_SerialPortHandlerCollection, l_VariablesMap["port"].as<uint16_t>());
        
        // Start event processing
        l_IoService.run();
        
        // Shutdown
        l_HdlcdServerHandlerCollection->Shutdown();
        l_SerialPortHandlerCollection->Shutdown();
        
    } catch (std::exception& a_Error) {
        std::cerr << "Exception: " << a_Error.what() << "\n";
        return 1;
    } // catch
    
    return 0;
}
