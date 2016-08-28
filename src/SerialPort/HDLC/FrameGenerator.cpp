/**
 * \file FrameGenerator.cpp
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

#include "FrameGenerator.h"
#include <assert.h>
#include "FCS16.h"

const std::vector<unsigned char> FrameGenerator::SerializeFrame(const Frame& a_Frame) {
    unsigned char l_ControlField = 0;
    bool l_bAppendPayload = false;
    switch (a_Frame.GetHDLCFrameType()) {
        case Frame::HDLC_FRAMETYPE_I: {
            l_ControlField = (((a_Frame.GetSSeq() & 0x07) << 1) | ((a_Frame.GetRSeq() & 0x07) << 5)); // I-Frame, PF=0
            l_bAppendPayload = true;            
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
            l_bAppendPayload = true;            
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
    
    // Assemble Frame
    std::vector<unsigned char> l_HDLCFrame;
    l_HDLCFrame.reserve(a_Frame.GetPayload().size() + 6); // 6 = FD, ADDR, TYPE, FCS, FCS, FD
    l_HDLCFrame.emplace_back(0x7E);
    l_HDLCFrame.emplace_back(a_Frame.GetAddress());
    if (a_Frame.IsPF()) {
        l_ControlField |= 0x10;
    } // if
    
    l_HDLCFrame.emplace_back(l_ControlField);
    if (l_bAppendPayload) {
        l_HDLCFrame.insert(l_HDLCFrame.end(), &(a_Frame.GetPayload())[0], &(a_Frame.GetPayload())[0] + a_Frame.GetPayload().size());
    } // if
    
    // Calculate FCS, perform escaping, and deliver
    ApplyFCS(l_HDLCFrame); // Plus 2 bytes
    l_HDLCFrame.emplace_back(0x7E);
    return std::move(l_HDLCFrame);
}

void FrameGenerator::ApplyFCS(std::vector<unsigned char> &a_HDLCFrame) {
    uint16_t trialfcs = pppfcs16(PPPINITFCS16, &a_HDLCFrame[1], (a_HDLCFrame.size() - 1));
    trialfcs ^= 0xffff;    
    a_HDLCFrame.emplace_back(trialfcs & 0x00ff);
    a_HDLCFrame.emplace_back((trialfcs >> 8) & 0x00ff);
}

std::vector<unsigned char> FrameGenerator::EscapeFrame(const std::vector<unsigned char> &a_HDLCFrame) {
    // Obtain required amount of memory for the fully escaped HDLC frame
    size_t l_NbrOfBytesToEscapeMax = 0;
    for (size_t l_Index = 1; l_Index < (a_HDLCFrame.size() - 1); ++l_Index) {
        if ((a_HDLCFrame[l_Index] == 0x7D) || (a_HDLCFrame[l_Index] == 0x7E)) {
            ++l_NbrOfBytesToEscapeMax;
        } // if
    } // for

    // Prepare return buffer
    std::vector<unsigned char> l_EscapedHDLCFrame;
    l_EscapedHDLCFrame.reserve(a_HDLCFrame.size() + l_NbrOfBytesToEscapeMax);
    l_EscapedHDLCFrame.emplace_back(0x7E);
    for (std::vector<unsigned char>::const_iterator it = (a_HDLCFrame.begin() + 1); it < (a_HDLCFrame.end() - 1); ++it) {
        if (*it == 0x7D) {
            l_EscapedHDLCFrame.emplace_back(0x7D);
            l_EscapedHDLCFrame.emplace_back(0x5D);
        } else if (*it == 0x7E) {
            l_EscapedHDLCFrame.emplace_back(0x7D);
            l_EscapedHDLCFrame.emplace_back(0x5E);
        } else {
            l_EscapedHDLCFrame.emplace_back(*it);
        } // else
    } // for
    
    l_EscapedHDLCFrame.emplace_back(0x7E);
    return std::move(l_EscapedHDLCFrame);
}
