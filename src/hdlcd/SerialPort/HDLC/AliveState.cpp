/**
 * \file AliveState.cpp
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
