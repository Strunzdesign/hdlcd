/**
 * \file LockGuard.cpp
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

#include "LockGuard.h"
#include "../SerialPort/SerialPortHandler.h"

LockGuard::LockGuard() {
    m_bLockedBySelf = false;
    m_bLockedByOthers = false;
    m_bLastLockedBySelf = false;
    m_bLastLockedByOthers = false;
}

LockGuard::~LockGuard() {
    if (m_bLockedBySelf) {
         m_SerialPortHandler->ResumeSerialPort();
    } // if
}

void LockGuard::Init(std::shared_ptr<SerialPortHandler> a_SerialPortHandler) {
    // Checks
    assert(m_bLockedBySelf == false);
    assert(m_bLockedByOthers == false);
    m_SerialPortHandler = a_SerialPortHandler;
}

void LockGuard::AcquireLock() {
    if (!m_bLockedBySelf) {
        // Lock it now!
        m_bLockedBySelf = true;
        m_SerialPortHandler->SuspendSerialPort();
    } // if
}

void LockGuard::ReleaseLock() {
    if (m_bLockedByOthers) {
        m_bLockedByOthers = false;
        m_SerialPortHandler->ResumeSerialPort();
    } // if
}

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
