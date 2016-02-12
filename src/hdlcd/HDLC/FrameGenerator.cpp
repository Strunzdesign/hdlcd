/**
 * \file FrameGenerator.cpp
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

#include "FrameGenerator.h"
#include "../ComPortHandler.h"
#include "../FCS16.h"

const std::vector<unsigned char> FrameGenerator::SerializeFrame(const Frame& a_Frame) {
    // Reserve memory for the fully escaped HDLC frame
    size_t l_NbrOfBytesToEscapeMax = 2; // worst-case if both FCS bytes have to be escaped
    for (size_t l_Index = 0; l_Index < a_Frame.GetPayload().size(); ++l_Index) {
        if ((a_Frame.GetPayload()[l_Index] == 0x7D) || (a_Frame.GetPayload()[l_Index] == 0x7E)) {
            ++l_NbrOfBytesToEscapeMax;
        } // if
    } // for

    // Reserve memory for the fully escaped HDLC frame
    size_t l_HDLCFrameSize = (a_Frame.GetPayload().size() + l_NbrOfBytesToEscapeMax + 6); // 6 = FD, ADDR, TYPE, FCS, FCS, FD
    std::vector<unsigned char> l_HDLCFrame(l_HDLCFrameSize);

    unsigned char l_ControlField = 0;
    size_t l_Offset = 2; // fits for nearly all S- and U-Frames (except UI), not including the frame delimiter
    switch (a_Frame.GetHDLCFrameType()) {
        case Frame::HDLC_FRAMETYPE_I: {
            l_ControlField = (((a_Frame.GetSSeq() & 0x07) << 1) | ((a_Frame.GetRSeq() & 0x07) << 5)); // I-Frame, PF=0
            memcpy(&l_HDLCFrame[3], &(a_Frame.GetPayload())[0], a_Frame.GetPayload().size());
            l_Offset = (a_Frame.GetPayload().size() + 2);
            break;
        }
        case Frame::HDLC_FRAMETYPE_S_RR: {
            l_ControlField = (0x01 | ((a_Frame.GetRSeq() & 0x07) << 5));
            break;
        }
        case Frame::HDLC_FRAMETYPE_S_RNR: {
            l_ControlField = (0x05 | ((a_Frame.GetRSeq() & 0x07) << 5));
            break;
        }
        case Frame::HDLC_FRAMETYPE_S_REJ: {
            l_ControlField = (0x09 | ((a_Frame.GetRSeq() & 0x07) << 5));
            break;
        }
        case Frame::HDLC_FRAMETYPE_S_SREJ: {
            l_ControlField = (0x0d | ((a_Frame.GetRSeq() & 0x07) << 5));
            break;
        }
        case Frame::HDLC_FRAMETYPE_U_UI: {
            l_ControlField = 0x03;
            assert(a_Frame.GetPayload().size() > 0);
            memcpy(&l_HDLCFrame[3], &(a_Frame.GetPayload())[0], a_Frame.GetPayload().size());
            l_Offset = (a_Frame.GetPayload().size() + 2);
            break;
        }
        case Frame::HDLC_FRAMETYPE_U_SABM: {
            l_ControlField = 0x2F;
            break;
        }
        case Frame::HDLC_FRAMETYPE_U_DISC: {
            l_ControlField = 0x43;
            break;
        }
        case Frame::HDLC_FRAMETYPE_U_UA: {
            l_ControlField = 0x63;
            break;
        }
        case Frame::HDLC_FRAMETYPE_U_CMDR: {
            l_ControlField = 0x83;
            break;
        }
        case Frame::HDLC_FRAMETYPE_U_TEST: {
            l_ControlField = 0xE3;
            break;
        }
        case Frame::HDLC_FRAMETYPE_U_SIM:
        case Frame::HDLC_FRAMETYPE_U_SARM:
        case Frame::HDLC_FRAMETYPE_U_UP:
        case Frame::HDLC_FRAMETYPE_U_SNRM:
        case Frame::HDLC_FRAMETYPE_U_XID: {
            // Unknown structure, not implemented yet
            assert(false);
            break;
        }
        default:
            assert(false);
    } // switch
    
    l_HDLCFrame[0] = 0x7E; // Frame delimiter
    l_HDLCFrame[1] = a_Frame.GetAddress();
    if (a_Frame.IsPF()) {
        l_ControlField |= 0x10;
    } // if
    
    l_HDLCFrame[2] = l_ControlField;
    
    // Calculate FCS, perform escaping, and deliver
    ApplyFCS(l_HDLCFrame, l_Offset); // 2 bytes without frame delimiter
    EscapeCharactersAndAddFE(l_HDLCFrame, l_NbrOfBytesToEscapeMax);
    return std::move(l_HDLCFrame);
}

void FrameGenerator::ApplyFCS(std::vector<unsigned char> &a_HDLCFrame, size_t a_NbrOfBytes) {
    uint16_t trialfcs = pppfcs16(PPPINITFCS16, &a_HDLCFrame[1], a_NbrOfBytes);
    trialfcs ^= 0xffff;
    a_HDLCFrame[a_NbrOfBytes + 1] = (trialfcs & 0x00ff);
    a_HDLCFrame[a_NbrOfBytes + 2] = ((trialfcs >> 8) & 0x00ff);
}

void FrameGenerator::EscapeCharactersAndAddFE(std::vector<unsigned char> &a_HDLCFrame, size_t a_NbrOfBytesToEscapeMax) {
    size_t l_NbrOfEscapedBytes = 0;
    size_t l_HDLCFrameSize = a_HDLCFrame.size();
    size_t l_Index = 1;
    for (; l_Index < (l_HDLCFrameSize - 1 - (a_NbrOfBytesToEscapeMax - l_NbrOfEscapedBytes)); ++l_Index) {
        if ((a_HDLCFrame[l_Index] == 0x7D) || (a_HDLCFrame[l_Index] == 0x7E)) {
            ++l_NbrOfEscapedBytes;
            memmove(&a_HDLCFrame[l_Index + 2], &a_HDLCFrame[l_Index + 1], (l_HDLCFrameSize - l_Index - 3));
            a_HDLCFrame[l_Index] = 0x7D;
            if (a_HDLCFrame[l_Index] == 0x7D) {
                a_HDLCFrame[l_Index + 1] = 0x5D;
            } else if (a_HDLCFrame[l_Index] == 0x7E) {
                a_HDLCFrame[l_Index + 1] = 0x5E;
            } // else if
            
            ++l_Index;
        } // if
    } // for
    
    // Resize vector and add frame delimiter
    a_HDLCFrame[l_Index] = 0x7E; // Frame delimiter
    a_HDLCFrame.resize(l_Index + 1);
}
