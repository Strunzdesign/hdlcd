/**
 * \file HdlcFrame.cpp
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

#include "HdlcFrame.h"
#include <sstream>
#include <iomanip> 

const std::vector<unsigned char> HdlcFrame::Dissect() const {
    bool l_bHasPayload = false;
    std::stringstream l_Output;
    if (IsIFrame()) {
        l_bHasPayload = true;
        l_Output << "HDLC frame, Addr=0x" << std::hex << (int)GetAddress() << std::dec << ", ";
        l_Output << "I-Frame, PF=" << IsPF() << ", SSeq=" << (int)GetSSeq() << ", RSeq=" << (int)GetRSeq();
    } else if (IsSFrame()) {
        l_Output << "HDLC frame, Addr=0x" << std::hex << (int)GetAddress() << std::dec << ", S-Frame: ";
        switch (GetHDLCFrameType()) {
            case HdlcFrame::HDLC_FRAMETYPE_S_RR: {
                l_Output << "RR";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_S_RNR: {
                l_Output << "RNR";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_S_REJ: {
                l_Output << "REJ";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_S_SREJ: {
                l_Output << "SREJ";
                break;
            }
            default: {
                break;
            }
        } // switch

        l_Output << ", PF=" << IsPF() << ", RSeq=" << (int)GetRSeq();
    } else if (IsUFrame()) {
        l_Output << "HDLC frame, Addr=0x" << std::hex << (int)GetAddress() << std::dec << ", U-Frame: ";
        switch (GetHDLCFrameType()) {
            case HdlcFrame::HDLC_FRAMETYPE_U_UI: {
                l_bHasPayload = true;
                l_Output << "UI";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_SIM: {
                l_Output << "SIM";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_SARM: {
                l_Output << "SARM";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_UP: {
                l_Output << "UP";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_SABM: {
                l_Output << "SABM";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_DISC: {
                l_Output << "DISC";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_UA: {
                l_Output << "UA";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_SNRM: {
                l_Output << "SNRM";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_CMDR: {
                l_bHasPayload = true;
                l_Output << "FRMR/CMDR";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_TEST: {
                l_bHasPayload = true;
                l_Output << "TEST";
                break;
            }
            case HdlcFrame::HDLC_FRAMETYPE_U_XID: {
                l_bHasPayload = true;
                l_Output << "XID";
                break;
            }
            default: {
                break;
            }
        } // switch
        
        l_Output << ", PF=" << IsPF();
    } else {
        l_Output << "Unparseable HDLC frame";
    } // else
    
    if (l_bHasPayload) {
        l_Output << ", with " << m_Payload.size() << " bytes payload:";
        for (auto it = m_Payload.begin(); it != m_Payload.end(); ++it) {
            l_Output << " " << std::hex << std::setw(2) << std::setfill('0') << int(*it);
        } // for
    } // if

    std::vector<unsigned char> l_DissectedFrame;
    std::string l_String(l_Output.str());
    l_DissectedFrame.insert(l_DissectedFrame.begin(), l_String.data(), (l_String.data() + l_String.size()));
    return l_DissectedFrame;
}
