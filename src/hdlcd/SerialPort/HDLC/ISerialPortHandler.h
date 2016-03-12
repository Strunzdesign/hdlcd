/**
 * \file ISerialPortHandler.h
 * \brief 
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

#ifndef ISERIAL_PORT_HANDLER_H
#define ISERIAL_PORT_HANDLER_H

#include <vector>
#include "BufferType.h"

class ISerialPortHandler {
public:
    // DTOR
    virtual ~ISerialPortHandler(){}

    // Methods called by the HDLC ProtocolState object
    virtual void DeliverBufferToClients(E_BUFFER_TYPE a_eBufferType, const std::vector<unsigned char> &a_Payload, bool a_bReliable, bool a_bValid, bool a_bWasSent) = 0;
    virtual void ChangeBaudRate() = 0;
    virtual void PropagateSerialPortState() = 0;
    virtual void TransmitHDLCFrame(const std::vector<unsigned char> &a_Payload) = 0;
    virtual void QueryForPayload(bool a_bQueryReliable, bool a_bQueryUnreliable) = 0;
};

#endif // ISERIAL_PORT_HANDLER_H
