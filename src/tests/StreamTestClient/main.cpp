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
#include <bitset>
#include <boost/asio.hpp>
#include "../../shared/AccessClient.h"

class Statistic {
public:
    Statistic(std::string a_Identifier): m_Identifier(a_Identifier)  {
        Reset();
    }
    
    void Reset() {
        m_SeqNbrsSeen[0].reset();
        m_SeqNbrsSeen[1].reset();
        m_Generation = 0;
        m_BankIndex = 0;
    }
    
    void Update(uint32_t a_SeqNbr) {
        uint32_t l_Generation = ((a_SeqNbr & 0xFFFFFF00) >> 8);
        uint8_t  l_Nbr        =  (a_SeqNbr & 0x000000FF);
        if (((l_Generation + 1) & 0x00FFFFFFF) == m_Generation) {
            // Ignore
        } else if (l_Generation == m_Generation) {
            // Copy
            m_SeqNbrsSeen[m_BankIndex].set(l_Nbr);
        } else if (l_Generation == ((m_Generation + 1) & 0x00FFFFFF)) {
            // Copy
            uint8_t l_BankIndex = ((m_BankIndex + 1) & 0x01);
            m_SeqNbrsSeen[l_BankIndex].set(l_Nbr);
        } else if (l_Generation == ((m_Generation + 2) & 0x00FFFFFF)) {
            // One generation is complete now
            PrintReport(m_Generation, m_SeqNbrsSeen[m_BankIndex].count());
            ++m_Generation;
            m_SeqNbrsSeen[m_BankIndex].reset();
            m_SeqNbrsSeen[m_BankIndex].set(l_Nbr);
            m_BankIndex = ((m_BankIndex + 1) & 0x01);
        } else if (l_Generation == ((m_Generation + 3) & 0x00FFFFFF)) {
            // Both generations are complete now
            PrintReport(m_Generation, m_SeqNbrsSeen[m_BankIndex].count());
            ++m_Generation;
            uint8_t l_BankIndex = ((m_BankIndex + 1) & 0x01);
            PrintReport(m_Generation, m_SeqNbrsSeen[m_BankIndex].count());
            ++m_Generation;
            m_SeqNbrsSeen[0].reset();
            m_SeqNbrsSeen[1].reset();
            m_SeqNbrsSeen[l_BankIndex].set(l_Nbr);
        } else {
            // Start from scratch
            m_SeqNbrsSeen[0].reset();
            m_SeqNbrsSeen[1].reset();
            m_BankIndex = 0;
            m_Generation = l_Generation;
            m_SeqNbrsSeen[0].set(l_Nbr);
        } // else
    }

private:
    // Helpers
    void PrintReport(uint32_t a_Generation, size_t a_AmountOfPackets) {
        std::cout << "Report " << std::dec << m_Identifier << ": Gen = " << a_Generation << ", Amount = " << a_AmountOfPackets;
        if (a_AmountOfPackets == 256) {
            std::cout << std::endl;
        } else {
            std::cout << " (!!!)" << std::endl;
        } // else
    }
    
    // Members
    std::string m_Identifier;
    uint32_t m_Generation;
    uint8_t m_BankIndex;
    std::bitset<256> m_SeqNbrsSeen[2];
};









class DataSource {
public:
    DataSource(boost::asio::io_service& a_IoService, AccessClient& a_AccessClient, uint16_t a_UnicastSSA): m_Timer(a_IoService), m_AccessClient(a_AccessClient), m_LocalSeqNbr(0), m_RemoteStatistic("remote      "), m_LocalStatistic("local       "), m_UnicastSSA(a_UnicastSSA) {
        srand(::time(NULL));
        m_LocalSeed = rand();
        m_RemoteSeed = 0;
        m_TotalBytesWritten = 0;
        StartTimer();
    }

    void PacketReceived(const PacketData& a_PacketData) {
        const std::vector<unsigned char> l_Buffer = a_PacketData.GetData();
        if ((l_Buffer[4] == ((m_UnicastSSA & 0xFF00) >> 8)) && (l_Buffer[5] == (m_UnicastSSA & 0x00FF)) && (l_Buffer[8] == 0x22) && (l_Buffer[8] == 0x22)) {
            if (l_Buffer[11] == 0x01) {
                HandleProbeReply(a_PacketData);
            } else if (l_Buffer[11] == 0x02) {
                HandlePeerStatistics(a_PacketData);
            } // if
        }
    }
    
private:
    // Helpers
    void HandleProbeReply(const PacketData& a_PacketData) {
        // Parsing
        const std::vector<unsigned char> l_Buffer = a_PacketData.GetData();
        uint32_t l_RemoteSeed = 0;
        l_RemoteSeed += (uint32_t(l_Buffer[12]) << 24);
        l_RemoteSeed += (uint32_t(l_Buffer[13]) << 16);
        l_RemoteSeed += (uint32_t(l_Buffer[14]) <<  8);
        l_RemoteSeed += (uint32_t(l_Buffer[15]));
        
        uint32_t l_LocalSeed = 0;
        l_LocalSeed += (uint32_t(l_Buffer[16]) << 24);
        l_LocalSeed += (uint32_t(l_Buffer[17]) << 16);
        l_LocalSeed += (uint32_t(l_Buffer[18]) <<  8);
        l_LocalSeed += (uint32_t(l_Buffer[19]));
        
        uint32_t l_RemoteSeqNbr = 0;
        l_RemoteSeqNbr += (uint32_t(l_Buffer[20]) << 24);
        l_RemoteSeqNbr += (uint32_t(l_Buffer[21]) << 16);
        l_RemoteSeqNbr += (uint32_t(l_Buffer[22]) <<  8);
        l_RemoteSeqNbr += (uint32_t(l_Buffer[23]));
        
        uint32_t l_LocalSeqNbr = 0;
        l_LocalSeqNbr += (uint32_t(l_Buffer[24]) << 24);
        l_LocalSeqNbr += (uint32_t(l_Buffer[25]) << 16);
        l_LocalSeqNbr += (uint32_t(l_Buffer[26]) <<  8);
        l_LocalSeqNbr += (uint32_t(l_Buffer[27]));
        
        UpdateStatistics(l_RemoteSeed, l_LocalSeed, l_RemoteSeqNbr, l_LocalSeqNbr);
    }
    
    void HandlePeerStatistics(const PacketData& a_PacketData) {
        // Parsing
        const std::vector<unsigned char> l_Buffer = a_PacketData.GetData();
        uint32_t l_RemoteSeed = 0;
        l_RemoteSeed += (uint32_t(l_Buffer[12]) << 24);
        l_RemoteSeed += (uint32_t(l_Buffer[13]) << 16);
        l_RemoteSeed += (uint32_t(l_Buffer[14]) <<  8);
        l_RemoteSeed += (uint32_t(l_Buffer[15]));
        
        uint32_t l_LocalSeed = 0;
        l_LocalSeed += (uint32_t(l_Buffer[16]) << 24);
        l_LocalSeed += (uint32_t(l_Buffer[17]) << 16);
        l_LocalSeed += (uint32_t(l_Buffer[18]) <<  8);
        l_LocalSeed += (uint32_t(l_Buffer[19]));
                
        uint32_t l_LocalGeneration = 0;
        l_LocalGeneration += (uint32_t(l_Buffer[20]) << 16);
        l_LocalGeneration += (uint32_t(l_Buffer[21]) <<  8);
        l_LocalGeneration += (uint32_t(l_Buffer[22]));
        
        uint16_t l_AmountOfPackets = 0;
        l_AmountOfPackets += (uint32_t(l_Buffer[23]) <<  8);
        l_AmountOfPackets += (uint32_t(l_Buffer[24]));
        
        // Print
        std::cout << "New peer statistics, Gen = " << l_LocalGeneration << ", Amount = " << l_AmountOfPackets;
        if (l_AmountOfPackets == 256) {
            std::cout << std::endl;
        } else {
            std::cout << " (!!!)" << std::endl;
        } // else
    }
    
    void UpdateStatistics(uint32_t a_RemoteSeed, uint32_t a_LocalSeed, uint32_t a_RemoteSeqNbr, uint32_t a_LocalSeqNbr) {
        // Update statistics
        if (m_LocalSeed != a_LocalSeed) {
            return;
        } // if
            
        if (m_RemoteSeed != a_RemoteSeed) {
            m_RemoteSeed = a_RemoteSeed;
            m_RemoteStatistic.Reset();
            m_LocalStatistic.Reset();
        } // if

        m_RemoteStatistic.Update(a_RemoteSeqNbr);
        m_LocalStatistic.Update(a_LocalSeqNbr);
    }

    bool SendNextProbeRequest() {
        // Create packet
        std::vector<unsigned char> l_Buffer;
        l_Buffer.emplace_back(0x00);
        l_Buffer.emplace_back(0x10);
        l_Buffer.emplace_back(0x40);
        l_Buffer.emplace_back(0x01);
        l_Buffer.emplace_back((m_UnicastSSA & 0xFF00) >> 8);
        l_Buffer.emplace_back (m_UnicastSSA & 0x00FF);
        l_Buffer.emplace_back(0x80);
        l_Buffer.emplace_back(0x00);
        l_Buffer.emplace_back(0x22);
        l_Buffer.emplace_back(0x22);
        l_Buffer.emplace_back(0x00);
        l_Buffer.emplace_back(0x00);
        l_Buffer.emplace_back((m_LocalSeed   & 0xFF000000) >> 24);
        l_Buffer.emplace_back((m_LocalSeed   & 0x00FF0000) >> 16);
        l_Buffer.emplace_back((m_LocalSeed   & 0x0000FF00) >>  8);
        l_Buffer.emplace_back (m_LocalSeed   & 0x000000FF);
        l_Buffer.emplace_back((m_LocalSeqNbr & 0xFF000000) >> 24);
        l_Buffer.emplace_back((m_LocalSeqNbr & 0x00FF0000) >> 16);
        l_Buffer.emplace_back((m_LocalSeqNbr & 0x0000FF00) >>  8);
        l_Buffer.emplace_back (m_LocalSeqNbr & 0x000000FF);

        if (m_AccessClient.Send(std::move(PacketData::CreatePacket(l_Buffer, true)))) {
            m_TotalBytesWritten += 20;
            if (((++m_LocalSeqNbr) % 0xFF) == 0) {
                std::cout << "256 Packets written (" << std::dec << m_TotalBytesWritten << " bytes total)" << std::endl;
            } // if
            
            return true;
        } // if
        
        return false;
    }
    
    void StartTimer() {
        m_Timer.expires_from_now(boost::posix_time::milliseconds(1));
        m_Timer.async_wait([this](const boost::system::error_code& a_ErrorCode) {
            if (!a_ErrorCode) {
                while (SendNextProbeRequest());
                StartTimer();
            } // if
        });
    }
    
    // Members
    uint16_t m_UnicastSSA;
    uint32_t m_RemoteSeed;
    uint32_t m_LocalSeed;
    AccessClient& m_AccessClient;
    uint32_t m_LocalSeqNbr;
    boost::asio::deadline_timer m_Timer;
    Statistic m_RemoteStatistic;
    Statistic m_LocalStatistic;
    unsigned long m_TotalBytesWritten;
};



int main(int argc, char* argv[]) {
    try {
        std::cerr << "Stream Test Client\n";
        if (argc != 5) {
            std::cerr << "Usage: StreamTestClient <host> <port> <usb-device> <Unicast-HEX-SSA>\n";
            return 1;
        } // if
        
        // Convert the provided hexadecimal SSA
        uint16_t l_UnicastSSA;
        std::istringstream l_Converter(argv[4]);
        l_Converter >> std::hex >> l_UnicastSSA;

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
        AccessClient l_AccessClient(io_service, endpoint_iterator, argv[3], 0x01);
        l_AccessClient.SetOnClosedCallback([&io_service](){io_service.stop();});

        // Prepare input
        DataSource l_DataSource(io_service, l_AccessClient, l_UnicastSSA);
        l_AccessClient.SetOnDataCallback([&l_DataSource](const PacketData& a_PacketData){ 
            l_DataSource.PacketReceived(a_PacketData);
        });

        // Start event processing
        io_service.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    } // catch
    
    return 0;
}
