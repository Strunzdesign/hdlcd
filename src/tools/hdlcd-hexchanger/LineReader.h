/**
 * \file LineReader.h
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

#ifndef LINE_READER_H
#define LINE_READER_H

#include <iostream>
#include <vector>
#include <boost/asio.hpp>

class LineReader {
public:
    // CTOR
    LineReader(boost::asio::io_service& io_service): m_InputStream(io_service, ::dup(STDIN_FILENO)), m_InputBuffer(4096), m_InputReader(&m_InputBuffer) {
        // Read single lines of input from STDIN
        do_read();
    }
    
    void SetOnInputLineCallback(std::function<void(const std::vector<unsigned char>)> a_OnInputLineCallback) {
        m_OnInputLineCallback = a_OnInputLineCallback;
    }
    
private:
    // Helpers
    void do_read() {
        boost::asio::async_read_until(m_InputStream, m_InputBuffer, '\n',[this](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                // Obtain one line from the input buffer, parse the provided hex dump, and create a StreamFrame from it
                char l_InputLineBuffer[1000];
                m_InputReader.getline(l_InputLineBuffer,1000);
                std::istringstream l_InputStream(l_InputLineBuffer);
                l_InputStream >> std::hex;
                std::vector<unsigned char> l_Buffer;
                l_Buffer.reserve(65536);
                l_Buffer.insert(l_Buffer.end(),std::istream_iterator<unsigned int>(l_InputStream), {});
                if (m_OnInputLineCallback) {
                    m_OnInputLineCallback(std::move(l_Buffer));
                } // if
                
                // Read the next line
                do_read();
            } else {
                // Some error occured
                m_InputStream.close();
            } // else
        });
    }

    // Members
    std::function<void(const std::vector<unsigned char>)> m_OnInputLineCallback;
    boost::asio::posix::stream_descriptor m_InputStream;
    boost::asio::streambuf m_InputBuffer;
    std::istream m_InputReader;
};

#endif // LINE_READER_H
