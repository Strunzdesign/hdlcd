/**
 * \file Frame.cpp
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

#include "Frame.h"
#include <sstream>
#include <iomanip> 

std::string Frame::GetReadableDescription() const {
    bool l_bHasPayload = false;
    std::stringstream l_Output;
    if (IsIFrame()) {
        l_bHasPayload = true;
        l_Output << "HDLC frame, Addr=" << (int)GetAddress() << ", ";
        l_Output << "I-Frame, PF=" << IsPF() << ", SSeq=" << (int)GetSSeq() << ", RSeq=" << (int)GetRSeq();
    } else if (IsSFrame()) {
        l_Output << "HDLC frame, Addr=" << (int)GetAddress() << ", S-Frame: ";
        switch (GetHDLCFrameType()) {
            case Frame::HDLC_FRAMETYPE_S_RR: {
                l_Output << "RR";
                break;
            }
            case Frame::HDLC_FRAMETYPE_S_RNR: {
                l_Output << "RNR";
                break;
            }
            case Frame::HDLC_FRAMETYPE_S_REJ: {
                l_Output << "REJ";
                break;
            }
            case Frame::HDLC_FRAMETYPE_S_SREJ: {
                l_Output << "SREJ";
                break;
            }
            default: {
                break;
            }
        } // switch

        l_Output << ", PF=" << IsPF() << ", RSeq=" << (int)GetRSeq();
    } else if (IsUFrame()) {
        l_Output << "HDLC frame, Addr=" << (int)GetAddress() << ", U-Frame: ";
        switch (GetHDLCFrameType()) {
            case Frame::HDLC_FRAMETYPE_U_UI: {
                l_bHasPayload = true;
                l_Output << "UI";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_SIM: {
                l_Output << "SIM";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_SARM: {
                l_Output << "SARM";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_UP: {
                l_Output << "UP";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_SABM: {
                l_Output << "SABM";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_DISC: {
                l_Output << "DISC";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_UA: {
                l_Output << "UA";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_SNRM: {
                l_Output << "SNRM";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_CMDR: {
                l_Output << "FRMR/CMDR";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_TEST: {
                l_Output << "TEST";
                break;
            }
            case Frame::HDLC_FRAMETYPE_U_XID: {
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

    return l_Output.str();
}
