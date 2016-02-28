/**
 * \file LogClientFormatter.h
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

#ifndef LOG_CLIENT_FORMATTER_H
#define LOG_CLIENT_FORMATTER_H

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <iostream>
#include <iomanip> 

void PrintLogEntry(const std::vector<unsigned char> &a_Buffer) {
    // Example: 19-02-2016;21:59:07.719;
    auto l_Now(boost::posix_time::microsec_clock::universal_time());
    auto l_Date(l_Now.date());
    auto l_DayTime (l_Now.time_of_day());
    std::cout << std::dec << l_Date.day() << "-"
                << std::setw(2) << std::setfill('0') << (int)l_Date.month() << "-"
                << std::setw(4) << std::setfill('0') << l_Date.year() << ";"
                << std::setw(2) << std::setfill('0') << l_DayTime.hours() << ":"
                << std::setw(2) << std::setfill('0') << l_DayTime.minutes() << ":"
                << std::setw(2) << std::setfill('0') << l_DayTime.seconds() << "."
                << std::setw(3) << std::setfill('0') << (l_DayTime.total_milliseconds() % 1000) << ";";
                
    // Print a hexdump of the provided data buffer. It should contain a packet to be printed in one line.
    for (auto it = a_Buffer.begin(); it != a_Buffer.end(); ++it) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << std::uppercase << int(*it) << " ";
    } // for

    std::cout << std::endl;
}

#endif // LOG_CLIENT_FORMATTER_H
