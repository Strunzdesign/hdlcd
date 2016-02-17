/**
 * \file Frame.h
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

#ifndef HDLC_FRAME_H
#define HDLC_FRAME_H

#include <string>
#include <vector>

class Frame {
public:
    Frame(): m_HDLCFrameType(HDLC_FRAMETYPE_UNSET), m_PF(false), m_RSeq(0), m_SSeq(0) {}
    
    void SetAddress(unsigned char a_Address) { m_Address = a_Address; }
    unsigned char GetAddress() const { return m_Address; }
    
    typedef enum {
        HDLC_FRAMETYPE_UNSET = 0,
        HDLC_FRAMETYPE_I,
        HDLC_FRAMETYPE_S_RR,
        HDLC_FRAMETYPE_S_RNR,
        HDLC_FRAMETYPE_S_REJ,
        HDLC_FRAMETYPE_S_SREJ,
        HDLC_FRAMETYPE_U_UI,
        HDLC_FRAMETYPE_U_SIM,
        HDLC_FRAMETYPE_U_SARM,
        HDLC_FRAMETYPE_U_UP,
        HDLC_FRAMETYPE_U_SABM,
        HDLC_FRAMETYPE_U_DISC,
        HDLC_FRAMETYPE_U_UA,
        HDLC_FRAMETYPE_U_SNRM,
        HDLC_FRAMETYPE_U_CMDR,
        HDLC_FRAMETYPE_U_TEST,
        HDLC_FRAMETYPE_U_XID,
    } T_HDLC_FRAMETYPE;
    void SetHDLCFrameType(T_HDLC_FRAMETYPE a_HDLCFrameType) { m_HDLCFrameType = a_HDLCFrameType; }
    T_HDLC_FRAMETYPE GetHDLCFrameType() const { return m_HDLCFrameType; }
    bool IsIFrame() const { return (m_HDLCFrameType == HDLC_FRAMETYPE_I); }
    bool IsSFrame() const { return ((m_HDLCFrameType >= HDLC_FRAMETYPE_S_RR) && (m_HDLCFrameType <= HDLC_FRAMETYPE_S_SREJ)); }
    bool IsUFrame() const { return ((m_HDLCFrameType >= HDLC_FRAMETYPE_U_UI) && (m_HDLCFrameType <= HDLC_FRAMETYPE_U_XID)); }
    
    void SetPF(bool a_PF) { if (a_PF) { m_PF = 0x10; } else { m_PF = 0; } }
    bool IsPF() const { return ( m_PF & 0x10); }
    
    void SetRSeq(unsigned char a_RSeq) { m_RSeq = a_RSeq; }
    unsigned char GetRSeq() const { return m_RSeq; }
    
    void SetSSeq(unsigned char a_SSeq) { m_SSeq = a_SSeq; }
    unsigned char GetSSeq() const { return m_SSeq; }
    
    void SetPayload(const std::vector<unsigned char> &a_Payload) { m_Payload = a_Payload; }
    const std::vector<unsigned char>& GetPayload() const { return m_Payload; }
    bool HasPayload() const { return (m_Payload.empty() == false); }
    
    std::string GetReadableDescription() const;
    
private:
    // Members
    unsigned char m_Address;
    T_HDLC_FRAMETYPE m_HDLCFrameType;
    unsigned char m_PF;
    unsigned char m_RSeq;
    unsigned char m_SSeq;
    std::vector<unsigned char> m_Payload;
};

#endif // HDLC_FRAME_H
