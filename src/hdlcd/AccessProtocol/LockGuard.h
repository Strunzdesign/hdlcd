/**
 * \file LockGuard.h
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

#ifndef LOCK_GUARD_H
#define LOCK_GUARD_H

#include <memory>
class SerialPortHandler;

class LockGuard {
public:
    // CTOR, DTOR, and initializer
    LockGuard();
    ~LockGuard();
    void Init(std::shared_ptr<SerialPortHandler> a_SerialPortHandler);
    
    // Influende the serial port
    void AcquireLock();
    void ReleaseLock();
    bool UpdateSerialPortState(size_t a_LockHolders);
    
    // Check status
    bool IsLocked() const { return (m_bLockedBySelf || m_bLockedByOthers); }
    bool IsLockedBySelf() const { return m_bLockedBySelf; }
    bool IsLockedByOthers() const { return m_bLockedByOthers; }

private:
    // Members
    std::shared_ptr<SerialPortHandler> m_SerialPortHandler;
    bool m_bLockedBySelf;
    bool m_bLockedByOthers;
    bool m_bLastLockedBySelf;
    bool m_bLastLockedByOthers;
};

#endif // LOCK_GUARD_H
