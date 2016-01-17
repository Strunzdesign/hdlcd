/**
 * \file FrameParser.cpp
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

#include "FrameParser.h"
#include "ProtocolState.h"
#include "../FCS16.h"

FrameParser::FrameParser(std::shared_ptr<ProtocolState> a_ProtocolState) {
    m_ProtocolState = a_ProtocolState;
    m_Bytes = 0;
    m_bStartTokenSeen = false;
}

void FrameParser::AddReceivedRawBytes(const char* a_Buffer, size_t a_Bytes) {
    while (a_Bytes) {
        size_t l_ConsumedBytes = AddChunk(a_Buffer, a_Bytes);
        a_Buffer += l_ConsumedBytes;
        a_Bytes  -= l_ConsumedBytes;
    } // while
}

size_t FrameParser::AddChunk(const char* a_Buffer, size_t a_Bytes) {
    if (m_bStartTokenSeen == false) {
        // No start token seen yet. 
        assert(m_Bytes == 0);
        
        // Check if there is the start token available in the input buffer.
        const void* l_pStartTokenPtr = memchr((const void*)a_Buffer, 0x7E, a_Bytes);
        if (l_pStartTokenPtr) {
            // The start token was found in the input buffer. 
            m_bStartTokenSeen = true;
            if (l_pStartTokenPtr == a_Buffer) {
                // The start token is at the beginning of the buffer. Just clip the start token.
                return 1;
            } else {
                // Clip front of buffer including the start token.
                return ((const char*)l_pStartTokenPtr - a_Buffer + 1);
            } // else
        } else {
            // No token found, and no token was seen yet. Dropping received buffer now.
            return a_Bytes;
        } // else
    } else {
        // We already have seen the start token. Check if there is the end token available in the input buffer.
        const void* l_pEndTokenPtr = memchr((const void*)a_Buffer, 0x7E, a_Bytes);
        if (l_pEndTokenPtr) {
            // The end token was found in the input buffer. Copy all bytes excluding the end token.
            size_t l_NbrOfBytes = ((const char*)l_pEndTokenPtr - a_Buffer);
            memcpy(&m_Buffer[m_Bytes], a_Buffer, l_NbrOfBytes);
            m_Bytes += l_NbrOfBytes;
            RemoveEscapeCharacters();
            return (l_NbrOfBytes + 1); // Including the consumed end token
        } else {
            // No end token found. Copy all bytes.
            memcpy(&m_Buffer[m_Bytes], a_Buffer, a_Bytes);
            m_Bytes += a_Bytes;
            return a_Bytes;
        } // else
    } // else
}

void FrameParser::RemoveEscapeCharacters() {
    // Checks
    assert(m_Bytes != 0x7E);
    assert(m_Buffer[m_Bytes - 1] != 0x7E);
    if (m_Bytes == 0) {
        return;
    } // if
    
    // Check for illegal escape sequence at the end of the buffer
    bool l_bMessageValid = true;
    if (m_Buffer[m_Bytes - 1] == 0x7D) {
        l_bMessageValid = false;
    } // if

    if (l_bMessageValid) {
        // Remove escape sequences
        for (size_t l_Index = 0; l_Index < (m_Bytes - 1); ++l_Index) {
            if (m_Buffer[l_Index] == 0x7D) {
                char l_Byte = m_Buffer[l_Index + 1];
                if (l_Byte == 0x5E) {
                    m_Buffer[l_Index] = 0x7E;
                    memmove(&m_Buffer[l_Index + 1], &m_Buffer[l_Index + 2], (m_Bytes - l_Index - 2));
                    --m_Bytes;
                } else if (l_Byte == 0x5D) {
                    m_Buffer[l_Index] = 0x7D;
                    memmove(&m_Buffer[l_Index + 1], &m_Buffer[l_Index + 2], (m_Bytes - l_Index - 2));
                    --m_Bytes;
                } else {
                    l_bMessageValid = false;
                    break;
                } // else
            } // if
        } // for
    } // if
    
    bool l_FrameIsValid = (pppfcs16(PPPINITFCS16, m_Buffer, m_Bytes) == PPPGOODFCS16);
    if (l_FrameIsValid) {
        std::cout << "GOOD ";
    } else {
        std::cout << "BAD ";
    } // else
    
    // Dump
    if (l_bMessageValid) {
        std::cout << "read " << m_Bytes << " Bytes: ";
    } else {
        std::cout << "read " << m_Bytes << " DEFECTIVE Bytes: ";
    } // else
    
    for (size_t l_Index = 0; l_Index < m_Bytes; ++l_Index) {
        std::cout << int(m_Buffer[l_Index]) << " ";
    } // for
    
    std::cout << std::endl;
    
    if (l_FrameIsValid) {
        m_ProtocolState->DeliverDeserializedFrame(DeserializeFrame());
    } // if
    
    // Remove consumed frame
    m_Bytes = 0;
    m_bStartTokenSeen = false;
}

Frame FrameParser::DeserializeFrame() const {
    // Parse byte buffer to get the HDLC frame
    Frame l_Frame;
    l_Frame.SetAddress(m_Buffer[0]);
    unsigned char l_ucCtrl = m_Buffer[1];
    l_Frame.SetPF((l_ucCtrl & 0x10) >> 4);
    if ((l_ucCtrl & 0x01) == 0) {
        // I-Frame
        l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_I);
        l_Frame.SetSSeq((l_ucCtrl & 0x0E) >> 1);
        l_Frame.SetRSeq((l_ucCtrl & 0xE0) >> 5);
        std::vector<unsigned char> l_Payload(m_Bytes - 4);
        memcpy(&l_Payload[0], &m_Buffer[2], (m_Bytes - 4));
        l_Frame.SetPayload(std::move(l_Payload));
    } else {
        // S-Frame or U-Frame
        if ((l_ucCtrl & 0x02) == 0x00) {
            // S-Frame    
            l_Frame.SetRSeq((l_ucCtrl & 0xE0) >> 5);
            unsigned char l_ucType = ((l_ucCtrl & 0x0c) >> 2);
            if (l_ucType == 0x00) {
                // Receive-Ready (RR)
                l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_S_RR);
            } else if (l_ucType == 0x01) {
                // Receive-Not-Ready (RNR)
                l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_S_RNR);
            } else if (l_ucType == 0x02) {
                // Reject (REJ)
                l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_S_REJ);
            } else {
                // Selective Reject (SREJ)
                l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_S_SREJ);
            } // else
        } else {
            // U-Frame
            unsigned char l_ucType = (((l_ucCtrl & 0x0c) >> 2) | ((l_ucCtrl & 0xe0) >> 3));
            switch (l_ucType) {
                case 0b00000: {
                    // Unnumbered information (UI)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_UI);
                    std::vector<unsigned char> l_Payload(m_Bytes - 4);
                    memcpy(&l_Payload[0], &m_Buffer[2], (m_Bytes - 4));
                    l_Frame.SetPayload(std::move(l_Payload));
                    break;
                }
                case 0b00001: {
                    // Set Init. Mode (SIM)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_SIM);
                    break;
                }
                case 0b00011: {
                    // Set Async. Response Mode (SARM)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_SARM);
                    break;
                }
                case 0b00100: {
                    // Unnumbered Poll (UP)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_UP);
                    break;
                }
                case 0b00111: {
                    // Set Async. Balance Mode (SABM)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_SABM);
                    break;
                }
                case 0b01000: {
                    // Disconnect (DISC)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_DISC);
                    break;
                }
                case 0b01100: {
                    // Unnumbered Ack. (UA)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_UA);
                    break;
                }
                case 0b10000: {
                    // Set normal response mode (SNRM)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_SNRM);
                    break;
                }
                case 0b10001: {
                    // Command reject (FRMR / CMDR)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_CMDR);
                    break;
                }
                case 0b11100: {
                    // Test (TEST)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_TEST);
                    break;
                }
                case 0b11101: {
                    // Exchange Identification (XID)
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_U_XID);
                    break;
                }
                default: {
                    l_Frame.SetHDLCFrameType(Frame::HDLC_FRAMETYPE_UNSET);
                    break;
                }
            } // switch
        } // else
    } // else
    
    return std::move(l_Frame);
}
