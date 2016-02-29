/**
 * \file AccessClient.h
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

#ifndef ACCESS_CLIENT_H
#define ACCESS_CLIENT_H

#include <boost/asio.hpp>
#include <vector>
#include <string>
#include "PacketEndpoint.h"
#include "PacketData.h"
#include "PacketCtrl.h"

class AccessClient {
public:
    // CTOR
    AccessClient(boost::asio::io_service& a_IoService, boost::asio::ip::tcp::resolver::iterator a_EndpointIterator, std::string a_SerialPortName, unsigned char a_SAP):
        m_TCPDataSocket(a_IoService),
        m_TCPCtrlSocket(a_IoService),
        m_PacketEndpointData(m_TCPDataSocket),
        m_PacketEndpointCtrl(m_TCPCtrlSocket),
        m_bDataSocketConnected(false),
        m_bCtrlSocketConnected(false),
        m_SerialPortName(a_SerialPortName),
        m_SAP(a_SAP) {
        // Callbacks: request data packets only from the data endpoint and control packets only from the control endpoint.
        // On any error, close everything!
        m_PacketEndpointData.SetOnDataCallback([this](const PacketData& a_PacketData){ OnDataReceived(a_PacketData); });
        m_PacketEndpointCtrl.SetOnCtrlCallback([this](const PacketCtrl& a_PacketCtrl){ OnCtrlReceived(a_PacketCtrl); });
        m_PacketEndpointData.SetOnClosedCallback([this](){ OnClosed(); });
        m_PacketEndpointCtrl.SetOnClosedCallback([this](){ OnClosed(); });
        
        // Connect data socket
        boost::asio::async_connect(m_TCPDataSocket, a_EndpointIterator,
                                   [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator) {
            if (!ec) {
                m_bDataSocketConnected = true;
                WriteSessionHeaders();
            } else {
                std::cerr << "Connect of data socket failed!" << std::endl;
                Close();
            } // else
        });
        
        // Connect control socket
        boost::asio::async_connect(m_TCPCtrlSocket, a_EndpointIterator,
                                   [this](boost::system::error_code ec, boost::asio::ip::tcp::resolver::iterator) {
            if (!ec) {
                m_bCtrlSocketConnected = true;
                WriteSessionHeaders();
            } else {
                std::cerr << "Connect of control socket failed!" << std::endl;
                Close();
            } // else
        });
    }
    
    void Shutdown() {
        m_PacketEndpointData.Shutdown();
        m_PacketEndpointCtrl.Shutdown();
    }
	
	void Close() {
	    m_PacketEndpointData.close();
		m_PacketEndpointCtrl.close();
	}
    
    // Callback methods
    void SetOnDataCallback(std::function<void(const PacketData& a_PacketData)> a_OnDataCallback) {
        m_OnDataCallback = a_OnDataCallback;
    }
    
    void SetOnCtrlCallback(std::function<void(const PacketCtrl& a_PacketCtrl)> a_OnCtrlCallback) {
        m_OnCtrlCallback = a_OnCtrlCallback;
    }
    
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }
    
    void Send(const PacketData& a_PacketData) {
        m_PacketEndpointData.Send(&a_PacketData);
    }

    void Send(const PacketCtrl& a_PacketCtrl) {
        m_PacketEndpointCtrl.Send(&a_PacketCtrl);
    }
    
private:
    // Internal callback methods
    void WriteSessionHeaders() {
        if ((!m_bDataSocketConnected) || (!m_bCtrlSocketConnected)) {
            return;
        } // if
        
        // Create session header for the data socket
        assert(m_SessionHeaderData.empty());
        m_SessionHeaderData.emplace_back(0x00); // Version 0
        m_SessionHeaderData.emplace_back(m_SAP); // As specified by the user. TODO: optimize usage if 0x10
        m_SessionHeaderData.emplace_back(m_SerialPortName.size()); // TODO: check if size fits into unsigned char
        m_SessionHeaderData.insert(m_SessionHeaderData.end(), m_SerialPortName.data(), (m_SerialPortName.data() + m_SerialPortName.size()));
        
        // Create session header for the control socket
        assert(m_SessionHeaderCtrl.empty());
        m_SessionHeaderCtrl.emplace_back(0x00); // Version 0
        m_SessionHeaderCtrl.emplace_back(0x10); // Control session
        m_SessionHeaderCtrl.emplace_back(m_SerialPortName.size()); // TODO: check if size fits into unsigned char
        m_SessionHeaderCtrl.insert(m_SessionHeaderCtrl.end(), m_SerialPortName.data(), (m_SerialPortName.data() + m_SerialPortName.size()));
        
        // Send session header via the data socket
        boost::asio::async_write(m_TCPDataSocket, boost::asio::buffer(m_SessionHeaderData.data(), m_SessionHeaderData.size()),
                                 [this](boost::system::error_code a_ErrorCode, std::size_t a_BytesWritten) {
            if (!a_ErrorCode) {
                // Continue with the exchange of packets
                m_PacketEndpointData.Start();
            } else {
                std::cerr << "Write of session header to the data socket failed!" << std::endl;
                Close();
            }
        });
        
        // Send session header via the control socket
        boost::asio::async_write(m_TCPCtrlSocket, boost::asio::buffer(m_SessionHeaderCtrl.data(), m_SessionHeaderCtrl.size()),
                                 [this](boost::system::error_code a_ErrorCode, std::size_t a_BytesWritten) {
            if (!a_ErrorCode) {
                // Continue with the echange of packets
                m_PacketEndpointCtrl.Start();
            } else {
                std::cerr << "Write of session header to the control socket failed!" << std::endl;
                Close();
            }
        });
    }
    
    void OnDataReceived(const PacketData& a_PacketData) {
        if (m_OnDataCallback) {
            m_OnDataCallback(a_PacketData);
        } // if
    }
    
    void OnCtrlReceived(const PacketCtrl& a_PacketCtrl) {
        if (m_OnCtrlCallback) {
            m_OnCtrlCallback(a_PacketCtrl);
        } // if
    }
    
    void OnClosed() {
        if (m_OnClosedCallback) {
            m_OnClosedCallback();
        } // if
    }
    
    // Members
    unsigned char m_SAP;
    boost::asio::ip::tcp::socket m_TCPDataSocket;
    boost::asio::ip::tcp::socket m_TCPCtrlSocket;
    bool m_bDataSocketConnected;
    bool m_bCtrlSocketConnected;
    
    // Both session headers to be transmitted
    std::vector<unsigned char> m_SessionHeaderData;
    std::vector<unsigned char> m_SessionHeaderCtrl;
    
    std::string m_SerialPortName;
    PacketEndpoint m_PacketEndpointData;
    PacketEndpoint m_PacketEndpointCtrl;
    
    // All possible callbacks for a user of this class
    std::function<void(const PacketData&)> m_OnDataCallback;
    std::function<void(const PacketCtrl&)> m_OnCtrlCallback;
    std::function<void()> m_OnClosedCallback;
};

#endif // ACCESS_CLIENT_H
