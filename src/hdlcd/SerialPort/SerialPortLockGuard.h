/**
 * \file SerialPortLockGuard.h
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

#ifndef SERIAL_PORT_LOCK_GUARD_H
#define SERIAL_PORT_LOCK_GUARD_H

#include <memory>
class SerialPortHandler;

class SerialPortLockGuard {
public:
    // CTOR, DTOR, and initializer
    SerialPortLockGuard();
    ~SerialPortLockGuard();
    void Init(std::shared_ptr<SerialPortHandler> a_SerialPortHandler);
    
    // Influende the serial port
    void SuspendSerialPort();
    void ResumeSerialPort();
    bool UpdateSerialPortStateX(size_t a_LockHolders);
    
    // Check status
    bool IsLocked() const { return (m_bLockedByOwn || m_bLockedByForeign); }
    bool IsLockedByOwn() const { return m_bLockedByOwn; }
    bool IsLockedByForeign() const { return m_bLockedByForeign; }

private:
    // Members
    std::shared_ptr<SerialPortHandler> m_SerialPortHandler;
    bool m_bLockedByOwn;
    bool m_bLockedByForeign;
    bool m_bLastLockedByOwn;
    bool m_bLastLockedByForeign;
};

#endif // SERIAL_PORT_LOCK_GUARD_H
