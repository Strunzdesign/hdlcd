/**
 * \file      LockGuard.h
 * \brief     This file contains the header declaration of class LockGuard
 * \author    Florian Evers, florian-evers@gmx.de
 * \copyright GNU Public License version 3.
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

/*! \class LockGuard
 *  \brief Class LockGuard
 * 
 *  This guard object tracks whether a related device attached via serial connections is currently "locked" / suspended or "unlocked" / resumed
 */
class LockGuard {
public:
    // CTOR, DTOR, and initializer
    LockGuard();
    ~LockGuard();
    void Init(std::shared_ptr<SerialPortHandler> a_SerialPortHandler);
    
    // Influende the serial port, obtain and release locks, called by a ClientHandler
    void AcquireLock();
    void ReleaseLock();
    
    // Update the effective state of a serial port
    bool UpdateSerialPortState(size_t a_LockHolders);
    
    
    /*! \brief Query the lock state of the related serial device
     * 
     *  Query the lock state of the related serial device
     * 
     *  \return bool indicates whether the serial port is currently "locked" / suspended or "unlocked" / resumed
     */
    bool IsLocked() const { return (m_bLockedBySelf || m_bLockedByOthers); }
    
    /*! \brief Query whether the serial device is currently "locked" by the responsible AccessClient entity
     * 
     *  Query whether the serial device is currently locked" by the responsible AccessClient entity
     * 
     *  \return bool indicates whether the serial port is currently "locked" by the responsible AccessClient entity
     */
    bool IsLockedBySelf() const { return m_bLockedBySelf; }
    
    /*! \brief Query whether the serial device is currently "locked" by at least one other AccessClient entity
     * 
     *  Query whether the serial device is currently "locked" by at least one other AccessClient entity
     * 
     *  \return bool indicates whether the serial port is currently "locked" by at least one other AccessClient entity
     */
    bool IsLockedByOthers() const { return m_bLockedByOthers; }

private:
    // Members
    std::shared_ptr<SerialPortHandler> m_SerialPortHandler; //!< The serial port handler responsible for the serial device
    bool m_bLockedBySelf;       //!< This flag indicates whether the serial device is locked by this entity
    bool m_bLockedByOthers;     //!< This flag indicates whether the serial device is locked by other entities
    bool m_bLastLockedBySelf;   //!< The last known effective state whether the serial device is locked by this entity, to detect changes
    bool m_bLastLockedByOthers; //!< The last known effective state whether the serial device is locked by other entities, to detect changes
};

#endif // LOCK_GUARD_H
