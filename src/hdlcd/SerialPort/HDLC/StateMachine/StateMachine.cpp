/**
 * \file StateMachine.cpp
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

#include "StateMachine.h"
#include <assert.h>

StateMachine::StateMachine(boost::asio::io_service& a_IOService, IDataLinkUser* a_pDataLinkUser):
    m_AwaitingConnectionState(this, a_pDataLinkUser, m_StateMembers),
    m_AwaitingReleaseState(this, a_pDataLinkUser, m_StateMembers),
    m_ConnectedState(this, a_pDataLinkUser, m_StateMembers),
    m_DisconnectedState(this, a_pDataLinkUser, m_StateMembers),
    m_TimerRecoveryState(this, a_pDataLinkUser, m_StateMembers) {
    // Checks
    assert(a_pDataLinkUser != NULL);

    // Initialize the state machine
    m_pCurrentState = &m_DisconnectedState;
}

StateMachine::~StateMachine() {
}

void StateMachine::Start() {
}

void StateMachine::Stop() {
}

void StateMachine::StateTransition(E_STATE a_eNextState) {
    switch (a_eNextState) {
        case STATE_DISCONNECTED:
            m_pCurrentState = &m_DisconnectedState;
            break;
        case STATE_AWAITING_CONNECTION:
            m_pCurrentState = &m_AwaitingConnectionState;
            break;
        case STATE_AWAITING_RELEASE:
            m_pCurrentState = &m_AwaitingReleaseState;
            break;
        case STATE_CONNECTED:
            m_pCurrentState = &m_ConnectedState;
            break;
        case STATE_TIMER_RECOVERY:
            m_pCurrentState = &m_TimerRecoveryState;
            break;
        default:
            assert(false);
            break;
    } // switch
}


void StateMachine::DLConnectRequest() {
    m_pCurrentState->ConnectRequest();
}

void StateMachine::DLDisconnectRequest() {
    m_pCurrentState->DisconnectRequest();
}

void StateMachine::DLFlowOffRequest() {
    m_pCurrentState->FlowOffRequest();
}

void StateMachine::DLFlowOnRequest() {
    m_pCurrentState->FlowOnRequest();
}
