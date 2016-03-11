/**
 * \file AliveState.cpp
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

#include "AliveState.h"
#include <assert.h>

AliveState::AliveState(boost::asio::io_service& a_IOService): m_StateTimer(a_IOService), m_ProbeTimer(a_IOService) {
    m_eAliveState = ALIVESTATE_PROBING;
    m_bFrameWasReceived = false;
    m_ProbeCounter = 0;
}

AliveState::~AliveState() {
    Stop();
}

void AliveState::Start() {
    assert(m_SendProbeCallback);
    assert(m_ChangeBaudrateCallback);
    m_ProbeCounter = 0;
    m_eAliveState = ALIVESTATE_PROBING;
    m_bFrameWasReceived = false;
    m_StateTimer.cancel();
    m_ProbeTimer.cancel();
    StartStateTimer();
    StartProbeTimer();
    m_SendProbeCallback(); // send first probe
}

void AliveState::Stop() {
    m_StateTimer.cancel();
    m_ProbeTimer.cancel();
}

void AliveState::SetSendProbeCallback(std::function<void()> a_SendProbeCallback) {
    m_SendProbeCallback = a_SendProbeCallback;
}

void AliveState::SetChangeBaudrateCallback(std::function<void()> a_ChangeBaudrateCallback) {
    m_ChangeBaudrateCallback = a_ChangeBaudrateCallback;
}

bool AliveState::OnFrameReceived() {
    m_bFrameWasReceived = true;
    if (m_eAliveState == ALIVESTATE_PROBING) {
        m_eAliveState = ALIVESTATE_FOUND;
        return true;
    } else {
        m_eAliveState = ALIVESTATE_FOUND;
        return false;
    } // else
}

bool AliveState::IsAlive() const {
    // Return true if transmission of data is currently allowed
    return ((m_eAliveState == ALIVESTATE_FOUND) || (m_eAliveState == ALIVESTATE_REPROBING));
}

void AliveState::StartStateTimer() {
    auto self(shared_from_this());
    m_StateTimer.expires_from_now(boost::posix_time::seconds(15));
    m_StateTimer.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            OnStateTimeout();
            StartStateTimer();
        } // if
    });
}

void AliveState::StartProbeTimer() {
    auto self(shared_from_this());
    m_ProbeTimer.expires_from_now(boost::posix_time::milliseconds(500));
    m_ProbeTimer.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            OnProbeTimeout();
            StartProbeTimer();
        } // if
    });
}

void AliveState::OnStateTimeout() {
    if (m_eAliveState == ALIVESTATE_FOUND) {
        if (m_bFrameWasReceived) {
            m_bFrameWasReceived = false;
        } else {
            m_eAliveState = ALIVESTATE_REPROBING;
        } // else
    } else if (m_eAliveState == ALIVESTATE_REPROBING) {
        if (!m_bFrameWasReceived) {
            m_eAliveState = ALIVESTATE_PROBING;
        } else {
            // This must not happen
            assert(false);
        } // else
    } // else
}

void AliveState::OnProbeTimeout() {
    if (m_eAliveState == ALIVESTATE_PROBING) {
        // For each fourth probe we select another baud rate
        if ((m_ProbeCounter = ((m_ProbeCounter + 1) & 0x03)) == 0) {
            m_ChangeBaudrateCallback();
        } // if

        m_SendProbeCallback();
    } else if (m_eAliveState == ALIVESTATE_REPROBING) {
        m_SendProbeCallback();
    } // else
}
