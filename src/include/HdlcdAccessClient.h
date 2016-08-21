/**
 * \file      HdlcdAccessClient.h
 * \brief 
 *
 * Copyright (c) 2016, Florian Evers, florian-evers@gmx.de
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     (1) Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 
 *     (2) Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.  
 *     
 *     (3)The name of the author may not be used to
 *     endorse or promote products derived from this software without
 *     specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HDLCD_ACCESS_CLIENT_H
#define HDLCD_ACCESS_CLIENT_H

#include <boost/asio.hpp>
#include <vector>
#include <string>
#include "HdlcdPacketEndpoint.h"
#include "HdlcdPacketData.h"
#include "HdlcdPacketCtrl.h"

/*! \class HdlcdAccessClient
 *  \brief Class HdlcdAccessClient
 * 
 *  The main helper class to easily implement clients of the HDLCd access protocol
 */
class HdlcdAccessClient {
public:
    /*! \brief The constructor of HdlcdAccessClient objects
     * 
     *  The connection is established on instantiation (RAII)
     * 
     *  \param a_IOService the boost IOService object
     *  \param a_EndpointIterator the boost endpoint iteratior referring to the destination
     *  \param a_SerialPortName the name of the serial port device
     *  \param a_SAP the numerical indentifier of the service access protocol
     */
    HdlcdAccessClient(boost::asio::io_service& a_IOService, boost::asio::ip::tcp::resolver::iterator a_EndpointIterator, std::string a_SerialPortName, unsigned char a_SAP):
        m_bClosed(false),
        m_TCPDataSocket(a_IOService),
        m_TCPCtrlSocket(a_IOService),
        m_bDataSocketConnected(false),
        m_bCtrlSocketConnected(false),
        m_SerialPortName(a_SerialPortName),
        m_SAP(a_SAP) {
        m_PacketEndpointData = std::make_shared<HdlcdPacketEndpoint>(m_TCPDataSocket);
        m_PacketEndpointCtrl = std::make_shared<HdlcdPacketEndpoint>(m_TCPCtrlSocket);
        // Callbacks: request data packets only from the data endpoint and control packets only from the control endpoint.
        // On any error, close everything!
        m_PacketEndpointData->SetOnDataCallback([this](std::shared_ptr<const HdlcdPacketData> a_PacketData){ return OnDataReceived(a_PacketData); });
        m_PacketEndpointCtrl->SetOnCtrlCallback([this](const HdlcdPacketCtrl& a_PacketCtrl){ OnCtrlReceived(a_PacketCtrl); });
        m_PacketEndpointData->SetOnClosedCallback([this](){ OnClosed(); });
        m_PacketEndpointCtrl->SetOnClosedCallback([this](){ OnClosed(); });
        
        // Connect data socket
        boost::asio::async_connect(m_TCPDataSocket, a_EndpointIterator,
                                   [this](boost::system::error_code a_ErrorCode, boost::asio::ip::tcp::resolver::iterator) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (!a_ErrorCode) {
                m_bDataSocketConnected = true;
                WriteSessionHeaders();
            } else {
                std::cerr << "Connect of data socket failed!" << std::endl;
                Close();
            } // else
        });
        
        // Connect control socket
        boost::asio::async_connect(m_TCPCtrlSocket, a_EndpointIterator,
                                   [this](boost::system::error_code a_ErrorCode, boost::asio::ip::tcp::resolver::iterator) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (!a_ErrorCode) {
                m_bCtrlSocketConnected = true;
                WriteSessionHeaders();
            } else {
                std::cerr << "Connect of control socket failed!" << std::endl;
                Close();
            } // else
        });
    }

    /*! \brief The destructor of AccessClient objects
     * 
     *  All open connections will automatically be closed by the destructor
     */
    ~HdlcdAccessClient() {
        m_OnDataCallback = NULL;
        m_OnCtrlCallback = NULL;
        m_OnClosedCallback = NULL;
        Close();
    }

    /*! \brief Shuts all TCP connections down
     * 
     *  Initiates a shutdown procedure for correct teardown of all TCP connections
     */
    void Shutdown() {
        m_PacketEndpointData->Shutdown();
        m_PacketEndpointCtrl->Shutdown();
    }

    /*! \brief Close the client entity
     * 
     *  Close the client entity and all its TCP connections
     */
    void Close() {
        if (m_bClosed == false) {
            m_bClosed = true;
            if (m_PacketEndpointData->WasStarted() == false) {
                m_TCPDataSocket.cancel();
                m_TCPDataSocket.close();
            } else {
                m_PacketEndpointData->Close();
            } // else
            
            if (m_PacketEndpointCtrl->WasStarted() == false) {
                m_TCPCtrlSocket.cancel();
                m_TCPCtrlSocket.close();
            } else {
                m_PacketEndpointCtrl->Close();
            } // else
            
            if (m_OnClosedCallback) {
                m_OnClosedCallback();
            } // if
        } // if
    }
    
    /*! \brief Provide a callback method to be called for received data packets
     * 
     *  Data packets are received in an asynchronous way. Use this method to specify a callback method to be called on reception of single data packets
     * 
     *  \param a_OnDataCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnDataCallback(std::function<void(const HdlcdPacketData& a_PacketData)> a_OnDataCallback) {
        m_OnDataCallback = a_OnDataCallback;
    }
    
    /*! \brief Provide a callback method to be called for received control packets
     * 
     *  Control packets are received in an asynchronous way. Use this method to specify a callback method to be called on reception of single control packets
     * 
     *  \param a_OnCtrlCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnCtrlCallback(std::function<void(const HdlcdPacketCtrl& a_PacketCtrl)> a_OnCtrlCallback) {
        m_OnCtrlCallback = a_OnCtrlCallback;
    }
    
    /*! \brief Provide a callback method to be called if this client entity is closing
     * 
     *  This method can be used to provide a function pointer callback to be called if this entoty is closing, e.g., the peer closed its endpoint
     * 
     *  \param a_OnClosedCallback the funtion pointer to the callback method, may be an empty function pointer to remove the callback
     */
    void SetOnClosedCallback(std::function<void()> a_OnClosedCallback) {
        m_OnClosedCallback = a_OnClosedCallback;
    }
    
    /*! \brief Send a single data packet to the peer entity
     * 
     *  Send a single data packet to the peer entity
     * 
     *  \param a_PacketData the data packet to be transmitted
     *  \param a_OnSendDoneCallback the callback handler to be called if the provided data packet was sent (optional)
     */
    bool Send(const HdlcdPacketData& a_PacketData, std::function<void()> a_OnSendDoneCallback = std::function<void()>()) {
        return m_PacketEndpointData->Send(&a_PacketData, a_OnSendDoneCallback);
    }

    /*! \brief Send a single control packet to the peer entity
     * 
     *  Send a single control packet to the peer entity
     * 
     *  \param a_PacketCtrl the control packet to be transmitted
     *  \param a_OnSendDoneCallback the callback handler to be called if the provided control packet was sent (optional)
     */
    bool Send(const HdlcdPacketCtrl& a_PacketCtrl, std::function<void()> a_OnSendDoneCallback = std::function<void()>()) {
        return m_PacketEndpointCtrl->Send(&a_PacketCtrl);
    }
    
private:
    /*! \brief Internal helper method to write the session headers to both outgoing TCP sockets
     * 
     *  This is an internal helper method to write the session headers to both outgoing TCP sockets
     */
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
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (!a_ErrorCode) {
                // Continue with the exchange of packets
                m_PacketEndpointData->Start();
            } else {
                std::cerr << "Write of session header to the data socket failed!" << std::endl;
                Close();
            }
        });
        
        // Send session header via the control socket
        boost::asio::async_write(m_TCPCtrlSocket, boost::asio::buffer(m_SessionHeaderCtrl.data(), m_SessionHeaderCtrl.size()),
                                 [this](boost::system::error_code a_ErrorCode, std::size_t a_BytesWritten) {
            if (a_ErrorCode == boost::asio::error::operation_aborted) return;
            if (!a_ErrorCode) {
                // Continue with the echange of packets
                m_PacketEndpointCtrl->Start();
            } else {
                std::cerr << "Write of session header to the control socket failed!" << std::endl;
                Close();
            }
        });
    }
    
    /*! \brief Internal callback method to be called on reception of data packets
     * 
     *  This is an internal callback method to be called on reception of data packets
     * 
     *  \param a_PacketData the received data packet
     */
    bool OnDataReceived(std::shared_ptr<const HdlcdPacketData> a_PacketData) {
        if (m_OnDataCallback) {
            m_OnDataCallback(*(a_PacketData.get()));
        } // if
        
        return true; // Do not stall the receiver
    }

    /*! \brief Internal callback method to be called on reception of control packets
     * 
     *  This is an internal callback method to be called on reception of control packets
     * 
     *  \param a_PacketCtrl the received data packet
     */
    void OnCtrlReceived(const HdlcdPacketCtrl& a_PacketCtrl) {
        if (m_OnCtrlCallback) {
            m_OnCtrlCallback(a_PacketCtrl);
        } // if
    }

    /*! \brief Internal callback method to be called on close of one of the TCP sockets
     * 
     *  This is an internal callback method to be called on close of one of the TCP sockets
     */
    void OnClosed() {
        Close();
    }
    
    // Members
    unsigned char m_SAP; //!< The service access point identifier that specifies the session type
    bool m_bClosed; //!< Indicates whether the HDLCd access protocol entity has already been closed
    boost::asio::ip::tcp::socket m_TCPDataSocket; //!< The TCP connection dedicated to user data
    boost::asio::ip::tcp::socket m_TCPCtrlSocket; //!< The TCP connection dedicated to control data
    bool m_bDataSocketConnected; //!< This flag indicates whether the user data socket is established
    bool m_bCtrlSocketConnected; //!< This flag indicates whether the user control socket is established
    
    // Both session headers to be transmitted
    std::vector<unsigned char> m_SessionHeaderData; //!< The buffer containing the session header for the data socket
    std::vector<unsigned char> m_SessionHeaderCtrl; //!< The buffer containing the session header for the control socket
    
    std::string m_SerialPortName; //!< The name of the serial port device
    std::shared_ptr<HdlcdPacketEndpoint> m_PacketEndpointData; //!< The packet endpoint class responsible for the connected data socket
    std::shared_ptr<HdlcdPacketEndpoint> m_PacketEndpointCtrl; //!< The packet endpoint class responsible for the connected control socket
    
    // All possible callbacks for a user of this class
    std::function<void(const HdlcdPacketData&)> m_OnDataCallback; //!< The callback function that is invoked on reception of a data packet
    std::function<void(const HdlcdPacketCtrl&)> m_OnCtrlCallback; //!< The callback function that is invoked on reception of a control packet
    std::function<void()> m_OnClosedCallback;  //!< The callback function that is invoked if the either this endpoint or that of the peer goes down
};

#endif // HDLCD_ACCESS_CLIENT_H
