/**
 * \file PacketCtrlPrinter.h
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

#ifndef PACKET_CTRL_PRINTER_H
#define PACKET_CTRL_PRINTER_H

#include <iostream>
#include "PacketCtrl.h"

void PacketCtrlPrinter(const PacketCtrl& a_PacketCtrl) {
    if (a_PacketCtrl.GetPacketType() == PacketCtrl::CTRL_TYPE_PORT_STATUS) {
        std::cout << "Serial port is: ";
        if (a_PacketCtrl.GetIsAlive()) {
            std::cout << "alive,     ";
        } else {
            std::cout << "not alive, ";
        } // else
        
        if (a_PacketCtrl.GetIsFlowSuspended()) {
            std::cout << "flow suspended, ";
        } else {
            std::cout << "flow open,      ";
        } // else
        
        if ((!a_PacketCtrl.GetIsLockedBySelf()) && (!a_PacketCtrl.GetIsLockedByOthers())) {
            std::cout << "without locks (resumed)" << std::endl;
        } else {
            if (a_PacketCtrl.GetIsLockedBySelf()) {
                std::cout << "locked with own lock,    ";
            } else {
                std::cout << "locked without own lock, ";
            } // else
            
            if (a_PacketCtrl.GetIsLockedByOthers()) {
                std::cout << "others have locks" << std::endl;
            } else {
                std::cout << "no other locks" << std::endl;
            } // else
        } // else
    } else if (a_PacketCtrl.GetPacketType() == PacketCtrl::CTRL_TYPE_ECHO) {
        std::cout << "Received an echo reply packet" << std::endl;
    } // else if
}


#endif // PACKET_CTRL_PRINTER_H
