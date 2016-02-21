/**
 * \file SerialPortLockGuard.cpp
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

#include "SerialPortLockGuard.h"
#include "SerialPortHandler.h"

SerialPortLockGuard::SerialPortLockGuard() {
    m_bLockedByOwn = false;
    m_bLockedByForeign = false;
    m_bLastLockedByOwn = false;
    m_bLastLockedByForeign = false;
}

SerialPortLockGuard::~SerialPortLockGuard() {
    if (m_bLockedByOwn) {
         m_SerialPortHandler->ResumeSerialPort();
    } // if
}

void SerialPortLockGuard::Init(std::shared_ptr<SerialPortHandler> a_SerialPortHandler) {
    // Checks
    assert(m_bLockedByOwn == false);
    assert(m_bLockedByForeign == false);
    m_SerialPortHandler = a_SerialPortHandler;
    m_bLockedByForeign = (m_SerialPortHandler->GetLockHolders() != 0);
}

void SerialPortLockGuard::SuspendSerialPort() {
    if (!m_bLockedByOwn) {
        // Lock it now!
        m_bLockedByOwn = true;
        m_SerialPortHandler->SuspendSerialPort();
    } // if
}

void SerialPortLockGuard::ResumeSerialPort() {
    if (m_bLockedByOwn) {
        m_bLockedByOwn = false;
        m_SerialPortHandler->ResumeSerialPort();
    } // if
}

bool SerialPortLockGuard::UpdateSerialPortStateX(size_t a_LockHolders) {
    // This call is caused by ourselves
    if (m_bLockedByOwn) {
        m_bLockedByForeign = (a_LockHolders > 1);
    } else {
        m_bLockedByForeign = (a_LockHolders > 0);
    } // else
    
    if ((m_bLastLockedByOwn     != m_bLockedByOwn) ||
        (m_bLastLockedByForeign != m_bLockedByForeign)) {
        m_bLastLockedByOwn     = m_bLockedByOwn;
        m_bLastLockedByForeign = m_bLockedByForeign;
        return true;
    } else {
        return false;
    } // else
}
