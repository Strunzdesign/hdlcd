/**
 * \file      AliveState.cpp
 * \brief     This file contains the implementation of class AliveState
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright BSD 3-Clause License.
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

/*! \brief The constructor of AliveState objects
 * 
 *  The constructor of AliveState objects
 */
AliveState::AliveState(boost::asio::io_service& a_IOService): m_StateTimer(a_IOService), m_ProbeTimer(a_IOService) {
    Reset();
}

/*! \brief The destructor of AliveState objects
 * 
 *  The destructor of AliveState objects
 */
AliveState::~AliveState() {
    Stop();
}

/*! \brief Internal helper to reset most of the members (excluding callbacks) to sane values
 * 
 *  Internal helper to reset most of the members (excluding callbacks) to sane values
 */
void AliveState::Reset() {
    m_eAliveState = ALIVESTATE_PROBING;
    m_bFrameWasReceived = false;
    m_ProbeCounter = 0;
}

/*! \brief Start all activities
 * 
 *  This method starts all activities of this class, such as sending probe request and testing different baud rate settings
 */
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

/*! \brief Stop all activities
 * 
 *  This method stops all activities of this class, such as sending probe request and testing different baud rate settings
 */
void AliveState::Stop() {
    m_StateTimer.cancel();
    m_ProbeTimer.cancel();
    Reset();
}

/*! \brief Register callback method for sending a probe request via HDLC
 * 
 *  The provided callback is called each time a probe has to be sent via HDLC.
 * 
 *  \param a_SendProbeCallback the callback method to send a probe request via HDLC
 */
void AliveState::SetSendProbeCallback(std::function<void()> a_SendProbeCallback) {
    m_SendProbeCallback = a_SendProbeCallback;
}

/*! \brief Register callback method for changing the baud rate of the related serial port
 * 
 *  The provided callback is called each time the baud rate of the serial port has to be changed.
 * 
 *  \param a_ChangeBaudrateCallback the callback method to switch to another baud rate setting
 */
void AliveState::SetChangeBaudrateCallback(std::function<void()> a_ChangeBaudrateCallback) {
    m_ChangeBaudrateCallback = a_ChangeBaudrateCallback;
}

/*! \brief Indicate that a HDLC frame was received via the related serial port
 * 
 *  On each valid received HDLC frame this method is called to indicate that the baud rate is ok and that
 *  the connected device is alive.
 * 
 *  \return bool indicates whether the internal state changed
 */
bool AliveState::OnFrameReceived() {
    // We received an HDLC frame
    m_bFrameWasReceived = true;
    if (m_eAliveState == ALIVESTATE_PROBING) {
        // Reprobing succeeded
        m_eAliveState = ALIVESTATE_FOUND;
        return true;
    } else {
        m_eAliveState = ALIVESTATE_FOUND;
        return false; // TODO: Why false? Seems to be correct, but check again and add a comment!
    } // else
}

/*! \brief Query whether the related serialport is currently considered alive
 * 
 *  A serial port is alive if the baud rate is correct and HDLC frames are received.
 * 
 *  \return bool indicates whether the related serial port is currently alive
 */
bool AliveState::IsAlive() const {
    // Return true if transmission of data is currently allowed
    return ((m_eAliveState == ALIVESTATE_FOUND) || (m_eAliveState == ALIVESTATE_REPROBING));
}

/*! \brief Internal helper method to start the state timeout timer
 * 
 *  Internal helper method to start the state timeout timer
 */
void AliveState::StartStateTimer() {
    auto self(shared_from_this());
    m_StateTimer.expires_from_now(boost::posix_time::seconds(15));
    m_StateTimer.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            // A state timeout occured
            OnStateTimeout();
            StartStateTimer(); // start again
        } // if
    });
}

/*! \brief Internal helper method to start the probe timeout timer
 * 
 *  Internal helper method to start the probe timeout timer
 */
void AliveState::StartProbeTimer() {
    auto self(shared_from_this());
    m_ProbeTimer.expires_from_now(boost::posix_time::milliseconds(500));
    m_ProbeTimer.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) {
            // A probe timeout occured
            OnProbeTimeout();
            StartProbeTimer(); // start again
        } // if
    });
}

/*! \brief Internal helper method to handle state timeouts
 * 
 *  This method implements the state machine regarding the alive state
 */
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

/*! \brief Internal helper method to handle probe timeouts
 * 
 *  This method triggers transmission of probe requests and toggles the baud rate if no HDLC frames were received
 */
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
