/**
 * \file HdlcdServerHandler.h
 * \brief 
 *
 * The HDLC Deamon implements the HDLC protocol to easily talk to devices connected via serial communications.
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

#ifndef HDLCD_SERVER_HANDLER_H
#define HDLCD_SERVER_HANDLER_H

#include <iostream>
#include <memory>
#include <string>
#include <deque>
#include <vector>
#include <boost/asio.hpp>
#include "AliveGuard.h"
#include "LockGuard.h"
#include "BufferType.h"
class Frame;
class HdlcdPacketData;
class HdlcdPacketCtrl;
class HdlcdPacketEndpoint;
class FrameEndpoint;
class HdlcdServerHandlerCollection;
class SerialPortHandler;
class SerialPortHandlerCollection;

class HdlcdServerHandler: public std::enable_shared_from_this<HdlcdServerHandler> {
public:
    HdlcdServerHandler(boost::asio::io_service& a_IOService, std::weak_ptr<HdlcdServerHandlerCollection> a_HdlcdServerHandlerCollection, boost::asio::ip::tcp::socket& a_TcpSocket);
    ~HdlcdServerHandler();
    
    E_BUFFER_TYPE GetBufferType() const { return m_eBufferType; }
    void DeliverBufferToClient(E_BUFFER_TYPE a_eBufferType, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bInvalid, bool a_bWasSent);
    void UpdateSerialPortState(bool a_bAlive, size_t a_LockHolders);
    void QueryForPayload(bool a_bQueryReliable, bool a_bQueryUnreliable);
    
    void Start(std::shared_ptr<SerialPortHandlerCollection> a_SerialPortHandlerCollection);
    void Stop();
    
private:
    // Callbacks
    bool OnFrame(const std::shared_ptr<Frame> a_Frame); // To parse the session header
    bool OnDataReceived(std::shared_ptr<const HdlcdPacketData> a_PacketData);
    void OnCtrlReceived(const HdlcdPacketCtrl& a_PacketCtrl);
    void OnClosed();

    // Members
    boost::asio::io_service& m_IOService;
    std::weak_ptr<HdlcdServerHandlerCollection> m_HdlcdServerHandlerCollection;
    std::shared_ptr<FrameEndpoint> m_FrameEndpoint;
    std::shared_ptr<HdlcdPacketEndpoint> m_PacketEndpoint;
    
    bool m_Registered;
    std::shared_ptr<SerialPortHandlerCollection> m_SerialPortHandlerCollection;
    std::shared_ptr<std::shared_ptr<SerialPortHandler>> m_SerialPortHandlerStopper;
    std::shared_ptr<SerialPortHandler> m_SerialPortHandler;
    
    // Pending incoming data packets
    bool m_bSerialPortHandlerAwaitsPacket;
    std::shared_ptr<const HdlcdPacketData> m_PendingIncomingPacketData;

    // Track the status of the serial port, communicate changes
    bool m_bDeliverInitialState;
    AliveGuard m_AliveGuard;
    LockGuard  m_LockGuard;

    // SAP specification
    E_BUFFER_TYPE m_eBufferType;
    bool m_bDeliverSent;
    bool m_bDeliverRcvd;
    bool m_bDeliverInvalidData;
};

#endif // HDLCD_SERVER_HANDLER_H
