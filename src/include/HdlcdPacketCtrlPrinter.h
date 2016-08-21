/**
 * \file HdlcdPacketCtrlPrinter.h
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

#ifndef HDLCD_PACKET_CTRL_PRINTER_H
#define HDLCD_PACKET_CTRL_PRINTER_H

#include <iostream>
#include "HdlcdPacketCtrl.h"

void HdlcdPacketCtrlPrinter(const HdlcdPacketCtrl& a_PacketCtrl) {
    if (a_PacketCtrl.GetPacketType() == HdlcdPacketCtrl::CTRL_TYPE_PORT_STATUS) {
        std::cout << "Serial port is: ";
        if (a_PacketCtrl.GetIsAlive()) {
            std::cout << "alive,     ";
        } else {
            std::cout << "not alive, ";
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
    } else if (a_PacketCtrl.GetPacketType() == HdlcdPacketCtrl::CTRL_TYPE_ECHO) {
        std::cout << "Received an echo reply packet" << std::endl;
    } // else if
}


#endif // HDLCD_PACKET_CTRL_PRINTER_H
