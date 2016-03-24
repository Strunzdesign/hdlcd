/**
 * \file DisconnectedState.cpp
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

#include "DisconnectedState.h"
#include "StateMachine.h"
#include "../IDataLinkUser.h"

DisconnectedState::DisconnectedState(StateMachine* a_pStateMachine, IDataLinkUser* a_pDataLinkUser, StateMembers& a_StateMembers):
    State(a_pStateMachine, a_pDataLinkUser, a_StateMembers) {
}

void DisconnectedState::ConnectRequest() {
    m_pStateMachine->StateTransition(STATE_AWAITING_CONNECTION);
}

void DisconnectedState::DisconnectRequest() {
    m_pDataLinkUser->DLDisconnectConfirmation();
}

void DisconnectedState::FlowOffRequest() {
    // Discard
}

void DisconnectedState::FlowOnRequest() {
    // Discard
}

void DisconnectedState::InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame, bool a_bMessageValid) {
    if (a_bMessageValid == false) {
        // TODO: support in frame parser is missing
        //m_pDataLinkUser->DLErrorIndication(ERROR_L_ControlFieldInvalidOrNotImplemented);
        return;
    } // if
    
    if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_U_UA) {
        // TODO: both make no sense
        m_pDataLinkUser->DLErrorIndication(ERROR_C_UnexpectedUAInStates345);
        m_pDataLinkUser->DLErrorIndication(ERROR_D_UAReceivedWithoutFWhenSABMOrDISCWasSentP);
        return;
    } // if
    
    if (a_Frame.GetHDLCFrameType() == Frame::HDLC_FRAMETYPE_U_UI) {
        UICheck(a_Payload, a_Frame);
        return;
    } // if
}

void DisconnectedState::TimerT1Expiry() {
    // Discard
}

void DisconnectedState::TimerT3Expiry() {
    // Discard
}
