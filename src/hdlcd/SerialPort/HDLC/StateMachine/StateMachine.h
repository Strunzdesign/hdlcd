/**
 * \file StateMachine.h
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

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <memory>
#include <boost/asio/deadline_timer.hpp>
#include "../IDataLinkLayer.h"
#include "StateMembers.h"
#include "AwaitingConnectionState.h"
#include "AwaitingReleaseState.h"
#include "ConnectedState.h"
#include "DisconnectedState.h"
#include "TimerRecoveryState.h"
class IDataLinkUser;

class StateMachine: public IDataLinkLayer, public std::enable_shared_from_this<StateMachine> {
public:
    StateMachine(boost::asio::io_service& a_IOService, IDataLinkUser* a_pDataLinkUser);
    ~StateMachine();

    void Start();
    void Stop();
    
    void StateTransition(E_STATE a_eNextState);
    
private:
    // Helpers: requests and responses issued by the higher lyer to the data link layer
    void DLConnectRequest();
    void DLDisconnectRequest();
    void DLFlowOffRequest();
    void DLFlowOnRequest();
    
    // Members
    AwaitingConnectionState m_AwaitingConnectionState;
    AwaitingReleaseState    m_AwaitingReleaseState;
    ConnectedState          m_ConnectedState;
    DisconnectedState       m_DisconnectedState;
    TimerRecoveryState      m_TimerRecoveryState;
    State*                  m_pCurrentState;
    
    // State members regarding the HDLC entities
    StateMembers            m_StateMembers;
};

#endif // STATE_MACHINE_H
