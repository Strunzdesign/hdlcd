/**
 * \file AliveState.h
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

#ifndef ALIVE_STATE_H
#define ALIVE_STATE_H

#include <memory>
#include <boost/asio/deadline_timer.hpp>

class AliveState: public std::enable_shared_from_this<AliveState> {
public:
    AliveState(boost::asio::io_service& a_IOService);
    ~AliveState();
    
    void SetSendProbeCallback(std::function<void()> a_SendProbeCallback);
    void SetChangeBaudrateCallback(std::function<void()> a_ChangeBaudrateCallback);
    
    void Start();
    void Stop();
    
    bool OnFrameReceived();
    bool IsAlive() const;
    
private:
    // Helpers
    void StartStateTimer();
    void StartProbeTimer();
    
    // Members
    typedef enum {
        ALIVESTATE_PROBING   = 0,
        ALIVESTATE_FOUND     = 1,
        ALIVESTATE_REPROBING = 2,
    } E_ALIVESTATE;
    E_ALIVESTATE m_eAliveState;
    
    // Callbacks
    std::function<void()> m_SendProbeCallback;
    std::function<void()> m_ChangeBaudrateCallback;
    
    // Timer and timer handlers
    boost::asio::deadline_timer m_StateTimer;
    boost::asio::deadline_timer m_ProbeTimer;
    void OnStateTimeout();
    void OnProbeTimeout();
    
    // Observation
    bool m_bFrameWasReceived;
    int m_ProbeCounter;
};

#endif // ALIVE_STATE_H
