/**
 * \file      AliveState.h
 * \brief     This file contains the header declaration of class AliveState
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

#ifndef ALIVE_STATE_H
#define ALIVE_STATE_H

#include <memory>
#include <boost/asio/deadline_timer.hpp>

/*! \class AliveState
 *  \brief Class AliveState
 * 
 *  This class is responsible for keeping track of the HDLC message exchange, periodically sends HDLC keep alive packets ("probes")
 *  if no other HDLC PDUs were exchanged, and cycles through all available baud rates if no messages were received from a serially
 *  attached device.
 */
class AliveState: public std::enable_shared_from_this<AliveState> {
public:
    // CTOR and DTOR
    AliveState(boost::asio::io_service& a_IOService);
    ~AliveState();
    
    // Register callback methods
    void SetSendProbeCallback(std::function<void()> a_SendProbeCallback);
    void SetChangeBaudrateCallback(std::function<void()> a_ChangeBaudrateCallback);
    
    // Start and stop processing
    void Start();
    void Stop();
    
    // Notification regarding incoming data, and query methods
    bool OnFrameReceived();
    bool IsAlive() const;
    
private:
    // Helpers
    void Reset();
    void StartStateTimer();
    void StartProbeTimer();
    
    // Members
    /*! \enum E_ALIVESTATE
     *  \brief Enum E_ALIVESTATE
     * 
     *  This enum names the different states that the baud rate and alive state detection scheme makes use of.
     */
    typedef enum {
        ALIVESTATE_PROBING   = 0, //!< The baud rate is currently unknown, only HDLC probes are sent. The serial port is not alive.
        ALIVESTATE_FOUND     = 1, //!< The baud rate is known, normal operation. The serial port is alive.
        ALIVESTATE_REPROBING = 2, //!< The baud rate is still known, but no packets were received. Reprobing is required to keep alive state.
    } E_ALIVESTATE;
    E_ALIVESTATE m_eAliveState;   //!< the current alive state
    
    // Callbacks
    std::function<void()> m_SendProbeCallback;      //!< This callback method is invoked if a subsequent probe has to be sent
    std::function<void()> m_ChangeBaudrateCallback; //!< This callback method is invoked if the baud rate has to be changed
    
    // Timer and timer handlers
    boost::asio::deadline_timer m_StateTimer; //!< Timeout timer for state handling
    boost::asio::deadline_timer m_ProbeTimer; //!< Timeout timer to trigger the next probe
    void OnStateTimeout();
    void OnProbeTimeout();
    
    // Observation
    bool m_bFrameWasReceived; //!< This flag indicates that data were received via HDLC since the last check
    int m_ProbeCounter;       //!< A counter to keep track of sent probes without the respective replies
};

#endif // ALIVE_STATE_H
