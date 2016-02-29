/**
 * \file ClientHandler.cpp
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

#include "ClientHandler.h"
#include "../SerialPort/SerialPortHandlerCollection.h"
#include "../SerialPort/SerialPortHandler.h"
#include "../../shared/PacketData.h"
#include "../../shared/PacketCtrl.h"

ClientHandler::ClientHandler(ClientHandlerCollection& a_ClientHandlerCollection, boost::asio::ip::tcp::socket a_TCPSocket):
    m_ClientHandlerCollection(a_ClientHandlerCollection),
    m_TCPSocket(std::move(a_TCPSocket)), m_PacketEndpoint(m_TCPSocket) {
    m_Registered = true;
    m_eHDLCBuffer = HDLCBUFFER_NOTHING;
    m_bDeliverSent = false;
    m_bDeliverRcvd = false;
    m_bDeliverInvalidData = false;
    m_PacketEndpoint.SetOnDataCallback([this](const PacketData& a_PacketData){ OnDataReceived(a_PacketData); });
    m_PacketEndpoint.SetOnCtrlCallback([this](const PacketCtrl& a_PacketCtrl){ OnCtrlReceived(a_PacketCtrl); });
    m_PacketEndpoint.SetOnClosedCallback([this](){ OnClosed(); });
}

void ClientHandler::DeliverBufferToClient(E_HDLCBUFFER a_eHDLCBuffer, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bValid, bool a_bWasSent) {
    // Check whether this buffer is of interest to this specific client
    bool l_bDeliver = (a_eHDLCBuffer == m_eHDLCBuffer);
    if ((a_bWasSent && !m_bDeliverSent) || (!a_bWasSent && !m_bDeliverRcvd)) {
        l_bDeliver = false;
    } // if
    
    if ((m_bDeliverInvalidData == false) && (a_bValid == false)) {
        l_bDeliver = false;
    } // if

    if (l_bDeliver) {
        auto l_Packet = PacketData(a_Payload, a_bReliable, a_bValid, a_bWasSent);
        m_PacketEndpoint.Send(&l_Packet);
    } // if
}

void ClientHandler::UpdateSerialPortState(size_t a_LockHolders) {
    if (m_SerialPortLockGuard.UpdateSerialPortState(a_LockHolders)) {
        // The state of the serial port state changed. Communicate the new state to the client.
        auto l_Packet = PacketCtrl(PacketCtrl::CTRL_TYPE_PORT_STATUS);
        l_Packet.SetIsAlive(true); // TODO
        l_Packet.SetFlowControl(true); // TODO
        l_Packet.SetIsLockedBySelf(m_SerialPortLockGuard.IsLockedBySelf());
        l_Packet.SetIsLockedByOthers(m_SerialPortLockGuard.IsLockedByOthers());
        m_PacketEndpoint.Send(&l_Packet);
    } // if
}

void ClientHandler::Start(std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection) {
    m_ClientHandlerCollection.RegisterClientHandler(shared_from_this());
    m_SerialPortHandlerCollection = a_SerialPortHandlerCollection;
    ReadSessionHeader1();
}

void ClientHandler::Stop() {
    if (m_Registered) {
        m_Registered = false;
        m_SerialPortHandler.reset();
        std::cerr << "TCP CLOSE" << std::endl;
        m_TCPSocket.close();
        m_ClientHandlerCollection.DeregisterClientHandler(shared_from_this());
    } // if
}

void ClientHandler::ReadSessionHeader1() {
    boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, 3),[this](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
            // Check version of the access protocol
            unsigned char l_Version = m_ReadBuffer[0];
            if (l_Version != 0x00) {
                // Unknown version of the access protocol
                std::cerr << "Unsupported access protocol version rejected: " << (int)l_Version << std::endl;
                m_TCPSocket.close();
                return;
            } // if
            
            // Check service access point specifier: type of data
            unsigned char l_SAP = m_ReadBuffer[1];
            switch (l_SAP & 0xF0) {
            case 0x00: {
                m_eHDLCBuffer = HDLCBUFFER_PAYLOAD;
                break;
            }
            case 0x10: {
                m_eHDLCBuffer = HDLCBUFFER_PORT_STATUS;
                break;
            }
            case 0x20: {
                m_eHDLCBuffer = HDLCBUFFER_PAYLOAD;
                break;
            }
            case 0x30: {
                m_eHDLCBuffer = HDLCBUFFER_RAW;
                break;
            }
            case 0x40: {
                m_eHDLCBuffer = HDLCBUFFER_DISSECTED;
                break;
            }
            default:
                // Unknown session type
                std::cerr << "Unknown session type rejected: " << (int)(l_SAP & 0xF0) << std::endl;
                m_TCPSocket.close();
                return;
            } // switch
            
            // Check service access point specifier: reserved bit
            if (l_SAP & 0x08) {
                // The reserved bit was set... aborting
                std::cerr << "Invalid reserved bit within SAP specifier of session header: " << (int)l_SAP << std::endl;
                m_TCPSocket.close();
                return;
            } // if
            
            // Check service access point specifier: invalids, deliver sent, and deliver rcvd
            m_bDeliverInvalidData = (l_SAP & 0x04); 
            m_bDeliverSent = (l_SAP & 0x02);
            m_bDeliverRcvd = (l_SAP & 0x01); 

            // Now read the name of the serial port
            ReadSessionHeader2(m_ReadBuffer[2]);
        } else {
            std::cerr << "TCP READ ERROR HEADER1:" << ec << std::endl;
            Stop();
        } // else
    });
}

void ClientHandler::ReadSessionHeader2(unsigned char a_BytesUSB) {
    boost::asio::async_read(m_TCPSocket, boost::asio::buffer(m_ReadBuffer, a_BytesUSB),[this](boost::system::error_code ec, std::size_t length) {
	    auto self(shared_from_this());
        if (!ec) {
            // Now we know the USB port
            std::string l_UsbPortString;
            l_UsbPortString.append((char*)m_ReadBuffer, length);
            m_SerialPortHandlerStopper = m_SerialPortHandlerCollection->GetSerialPortHandler(l_UsbPortString, self);
			if (m_SerialPortHandlerStopper) {
				m_SerialPortHandler = (*m_SerialPortHandlerStopper.get());
				m_SerialPortLockGuard.Init(m_SerialPortHandler);

				// Start the PacketEndpoint. It takes full control over the TCP socket.
				m_PacketEndpoint.Start();
		    } else {
				Stop();
			} // else
        } else {
            std::cerr << "TCP READ ERROR:" << ec << std::endl;
            Stop();
        } // else
    });
}

void ClientHandler::OnDataReceived(const PacketData& a_PacketData) {
    // TODO: check suspended state. Check whether to stall the TCP socket.
    m_SerialPortHandler->DeliverPayloadToHDLC(a_PacketData.GetData());
}

void ClientHandler::OnCtrlReceived(const PacketCtrl& a_PacketCtrl) {
    // TODO: check control packet: suspend / resume? Port kill request?
}

void ClientHandler::OnClosed() {
    Stop();
}
