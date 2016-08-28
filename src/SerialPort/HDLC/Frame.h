/**
 * \file Frame.h
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

#ifndef HDLC_FRAME_H
#define HDLC_FRAME_H

#include <string>
#include <vector>

class Frame {
public:
    Frame(): m_eHDLCFrameType(HDLC_FRAMETYPE_UNSET), m_PF(false), m_RSeq(0), m_SSeq(0) {}
    
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
    } E_HDLC_FRAMETYPE;
    void SetHDLCFrameType(E_HDLC_FRAMETYPE a_eHDLCFrameType) { m_eHDLCFrameType = a_eHDLCFrameType; }
    E_HDLC_FRAMETYPE GetHDLCFrameType() const { return m_eHDLCFrameType; }
    bool IsEmpty() const { return m_eHDLCFrameType == HDLC_FRAMETYPE_UNSET; }
    bool IsIFrame() const { return (m_eHDLCFrameType == HDLC_FRAMETYPE_I); }
    bool IsSFrame() const { return ((m_eHDLCFrameType >= HDLC_FRAMETYPE_S_RR) && (m_eHDLCFrameType <= HDLC_FRAMETYPE_S_SREJ)); }
    bool IsUFrame() const { return ((m_eHDLCFrameType >= HDLC_FRAMETYPE_U_UI) && (m_eHDLCFrameType <= HDLC_FRAMETYPE_U_XID)); }
    
    void SetPF(bool a_PF) { if (a_PF) { m_PF = 0x10; } else { m_PF = 0; } }
    bool IsPF() const { return ( m_PF & 0x10); }
    
    void SetRSeq(unsigned char a_RSeq) { m_RSeq = a_RSeq; }
    unsigned char GetRSeq() const { return m_RSeq; }
    
    void SetSSeq(unsigned char a_SSeq) { m_SSeq = a_SSeq; }
    unsigned char GetSSeq() const { return m_SSeq; }
    
    void SetPayload(const std::vector<unsigned char> &a_Payload) { m_Payload = a_Payload; }
    const std::vector<unsigned char>& GetPayload() const { return m_Payload; }
    bool HasPayload() const { return (m_Payload.empty() == false); }
    
    const std::vector<unsigned char> Dissect() const;
    
private:
    // Members
    unsigned char m_Address;
    E_HDLC_FRAMETYPE m_eHDLCFrameType;
    unsigned char m_PF;
    unsigned char m_RSeq;
    unsigned char m_SSeq;
    std::vector<unsigned char> m_Payload;
};

#endif // HDLC_FRAME_H
