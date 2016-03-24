/**
 * \file State.cpp
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

#include "State.h"
#include <assert.h>
#include "../IDataLinkUser.h"

State::State(StateMachine* a_pStateMachine, IDataLinkUser* a_pDataLinkUser, StateMembers& a_StateMembers):
    m_pStateMachine(a_pStateMachine),
    m_pDataLinkUser(a_pDataLinkUser),
    m_StateMembers(a_StateMembers) {
    // Checks
    assert(a_pStateMachine != NULL);
    assert(a_pDataLinkUser != NULL);
}

void State::NRErrorRecovery() {
    // 100% according to specification
    m_pDataLinkUser->DLErrorIndication(ERROR_J_NRSequenceError);
    EstablishDataLink();
    m_StateMembers.m_bLayer3Initiated = false;
}

void State::EstablishDataLink() {
    // 100% according to specification
    ClearExceptionConditions();
    m_StateMembers.m_RetryCounter = 0;
    m_StateMembers.m_bNextCommandPoll = true;
    // TODO: Trigger tranmission of SABM command with P=1
    // TODO: Stop T3 Timer
    // TODO: (Re)start T1 Timer
}

void State::ClearExceptionConditions() {
    // Nearly(!) 100% according to specification
    m_StateMembers.m_bPeerReceiverBusy = false;
    m_StateMembers.m_bOwnReceiverBusy = false;
    m_StateMembers.m_bRejectException = false;
    m_StateMembers.m_bSelectiveRejectException = false; // this was not named in the specification, but makes sense!
    m_StateMembers.m_bAcknowledgePending = false;
}

void State::TransmitEnquiry() {
    // Not clear yet: send queue required
}

void State::EnquiryResponse() {
    // Not clear yet: send queue required. Command wrong, must be response!
}

void State::InvokeRetransmission() {
    // Not clear yet: send queue required
}

void State::CheckIFrameAcknowledged(const Frame& a_Frame) {
    // 100% according to specification
    assert(a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_I);
    if (m_StateMembers.m_bPeerReceiverBusy) {
        m_StateMembers.m_AcknowledgeState = a_Frame.GetRSeq();
        // TODO: Start T3 timer
        if (true /* TODO: Timer T1 was not started */) {
            // TODO: Start T1 timer
        } // if
    } else {
        if (a_Frame.GetRSeq() == m_StateMembers.m_SendState) {
            m_StateMembers.m_AcknowledgeState = a_Frame.GetRSeq();
            // TODO: Stop T1 timer
            // TODO: Start T3 timer
            SelectT1Value();
        } else {
            if (a_Frame.GetRSeq() != m_StateMembers.m_AcknowledgeState) {
                m_StateMembers.m_AcknowledgeState = a_Frame.GetRSeq();
                // TODO: Restart T1 timer
            } // if
        } // else
    } // else
}

void State::CheckNeedForResponse(const Frame& a_Frame) {
    // 100% according to specification
    if (a_Frame.IsCommand() && a_Frame.IsPF()) {
        m_StateMembers.m_bNextResponseFinal = true;
        EnquiryResponse();
    } else {
        if ((a_Frame.IsCommand() == false) && a_Frame.IsPF()) {
            m_pDataLinkUser->DLErrorIndication(ERROR_A_FReceivedButPnotOutstanding);
        } // if
    } // else
}

void State::UICheck(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame) {
    // Nearly(!) 100% according to specification
    assert(a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_U_UI);
    if (a_Frame.IsCommand() == false) {
        m_pDataLinkUser->DLErrorIndication(ERROR_Q_UIResponseReceivedOrUICommandWithPReceived); // TODO: Check if second half makes sense
    } else {
        if (true /* TODO: if length limitation is okay */) {
            m_pDataLinkUser->DLUnitDataIndication(a_Payload);
        } else {
            m_pDataLinkUser->DLErrorIndication(ERROR_R_UIFrameExceededMaximumAllowedLength); // Wrong error code in the specifikation (K?)
        } // else
    } // else
}

void State::SelectT1Value() {
    // 100% according to specification
    if (m_StateMembers.m_RetryCounter == 0) {
        m_StateMembers.m_SmoothedRoundTripTimeMSecs = ((7 * (m_StateMembers.m_SmoothedRoundTripTimeMSecs       >> 3)) + 
                                                       (1 * (m_StateMembers.m_RemainingTimeOnT1WhenLastStopped >> 3)));
        m_StateMembers.m_NextValueForT1MSecs = (m_StateMembers.m_SmoothedRoundTripTimeMSecs << 1);
    } else {
        if (true /* TODO: if timer T1 expired */) {
            assert(m_StateMembers.m_RetryCounter < 4); // TODO: assure everywhere
            m_StateMembers.m_NextValueForT1MSecs = ((1 << (m_StateMembers.m_RetryCounter + 1)) * m_StateMembers.m_SmoothedRoundTripTimeMSecs);
        } // if
    } // else
}

void State::SetVersion() {
    // TODO: not clear
}
