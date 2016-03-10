/**
 * \file AliveState.h
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
