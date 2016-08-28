/**
 * \file      LockGuard.cpp
 * \brief     This file contains the implementation of class LockGuard
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright GNU Public License version 3.
 *
 * The HDLC Deamon implements the HDLC protocol to easily talk to devices connected via serial communications.
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

#include "LockGuard.h"
#include "../SerialPort/SerialPortHandler.h"

/*! \brief The constructor of LockGuard objects
 * 
 *  On creation, the serial port is "unlocked" / resumed
 */
LockGuard::LockGuard() {
    // No active locks known on creation
    m_bLockedBySelf = false;
    m_bLockedByOthers = false;
    m_bLastLockedBySelf = false;
    m_bLastLockedByOthers = false;
}

/*! \brief The destructor of LockGuard objects
 * 
 *  Held locks are automatically released on deletion
 */
LockGuard::~LockGuard() {
    if (m_bLockedBySelf) {
         m_SerialPortHandler->ResumeSerialPort();
    } // if
}

/*! \brief Initializer method to register subsequent objects
 * 
 *  Registers the serial port handler object that is not available yet on construction
 * 
 *  \param a_SerialPortHandler the serial port handler object
 */
void LockGuard::Init(std::shared_ptr<SerialPortHandler> a_SerialPortHandler) {
    // Checks
    assert(m_bLockedBySelf == false);
    assert(m_bLockedByOthers == false);
    m_SerialPortHandler = a_SerialPortHandler;
}

/*! \brief Method to aquire a lock
 * 
 *  Each ClientAcceptor can aquired a lock only once, subsequent calls have no impact. The serial port handler
 *  has a lock counter, so only the first lock attempt will suspend the serial port.
 */
void LockGuard::AcquireLock() {
    if (!m_bLockedBySelf) {
        // Increases lock counter, may actually suspend the serial port
        m_bLockedBySelf = true;
        m_SerialPortHandler->SuspendSerialPort();
    } // if
}

/*! \brief Method to release a lock
 * 
 *  Each ClientAcceptor can release a held lock only once, subsequent calls have no impact. The serial port handler
 *  has a lock counter, so it may resume after this call, but this happens only if this was the last lock.
 */
void LockGuard::ReleaseLock() {
    if (m_bLockedBySelf) {
        // Decreases lock counter, may actually resume the serial port
        m_bLockedBySelf = false;
        m_SerialPortHandler->ResumeSerialPort();
    } // if
}

/*! \brief Update the effective lock state of the related serial port
 * 
 *  This method gets the amount of locks regarding the related serial port. Thus, this entity is able to derive
 *  whether subsequent locks by other entities exist.
 * 
 *  \param  a_LockHolders the amount of locks that currently force the related serial port to be suspended
 *  \return bool indicates whether the internal state of this object changed
 */
bool LockGuard::UpdateSerialPortState(size_t a_LockHolders) {
    // This call is caused by ourselves
    if (m_bLockedBySelf) {
        m_bLockedByOthers = (a_LockHolders > 1);
    } else {
        m_bLockedByOthers = (a_LockHolders > 0);
    } // else
    
    if ((m_bLastLockedBySelf   != m_bLockedBySelf) ||
        (m_bLastLockedByOthers != m_bLockedByOthers)) {
        m_bLastLockedBySelf     = m_bLockedBySelf;
        m_bLastLockedByOthers   = m_bLockedByOthers;
        return true;
    } else {
        return false;
    } // else
}
