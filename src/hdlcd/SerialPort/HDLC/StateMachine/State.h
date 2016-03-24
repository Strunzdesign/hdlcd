/**
 * \file State.h
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

#ifndef STATE_H
#define STATE_H

#include <vector>
#include "../Frame.h"
#include "../ErrorCodes.h"
#include "StateMembers.h"
class StateMachine;
class IDataLinkUser;

typedef enum {
    STATE_DISCONNECTED        = 0,
    STATE_AWAITING_CONNECTION = 1,
    STATE_AWAITING_RELEASE    = 2,
    STATE_CONNECTED           = 3,
    STATE_TIMER_RECOVERY      = 4
} E_STATE;

class State {
public:
    State(StateMachine* a_pStateMachine, IDataLinkUser* a_pDataLinkUser, StateMembers& a_StateMembers);
    virtual ~State(){}
    
    // Requests from the "higher layer"
    virtual void ConnectRequest() = 0;
    virtual void DisconnectRequest() = 0;
    virtual void FlowOffRequest() = 0;
    virtual void FlowOnRequest() = 0;
    
    // Events
    virtual void InterpretDeserializedFrame(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame, bool a_bMessageValid) = 0;
    virtual void TimerT1Expiry() = 0;
    virtual void TimerT3Expiry() = 0;

protected:
    // Data link subroutines
    void NRErrorRecovery();
    void EstablishDataLink();
    void ClearExceptionConditions();
    void TransmitEnquiry();
    void EnquiryResponse();
    void InvokeRetransmission();
    void CheckIFrameAcknowledged(const Frame& a_Frame);
    void CheckNeedForResponse(const Frame& a_Frame);
    void UICheck(const std::vector<unsigned char> &a_Payload, const Frame& a_Frame);
    void SelectT1Value();
    void SetVersion(); // TODO: rename
    
    // Members
    StateMachine*  m_pStateMachine;
    IDataLinkUser* m_pDataLinkUser;
    StateMembers&  m_StateMembers;
};

#endif // STATE_H
