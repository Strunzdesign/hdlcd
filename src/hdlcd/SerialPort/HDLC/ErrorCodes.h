/**
 * \file ErrorCodes.h
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

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

typedef enum {
    ERROR_A_FReceivedButPnotOutstanding                =  0,
    ERROR_B_UnexpectedDMWithFinStates345               =  1,
    ERROR_C_UnexpectedUAInStates345                    =  2,
    ERROR_D_UAReceivedWithoutFWhenSABMOrDISCWasSentP   =  3,
    ERROR_E_DMReceivedInStates345                      =  4,
    ERROR_F_DataLinkReset                              =  5,
    ERROR_I_N2TimeoutsUnacknowledgedData               =  6,
    ERROR_J_NRSequenceError                            =  7,
    ERROR_L_ControlFieldInvalidOrNotImplemented        =  8,
    ERROR_M_InformationFieldWasReceivedInUOrSTypeFrame =  9,
    ERROR_N_LengthOfFrameIncorrectForFrameType         = 10,
    ERROR_O_IFrameExceededMaximumAllowedLength         = 11,
    ERROR_P_NSOutOfTheWindow                           = 12,
    ERROR_Q_UIResponseReceivedOrUICommandWithPReceived = 13,
    ERROR_R_UIFrameExceededMaximumAllowedLength        = 14,
    ERROR_S_IResponseReceived                          = 15,
    ERROR_T_N2TimeoutsNoResponseToEnquiry              = 16,
    ERROR_U_N2TimeoutsExtendedPeerBusyCondition        = 17,
    ERROR_V_NoDLMachinesAvailableToEstablishConnection = 18
} E_ERROR;

#endif // ERROR_CODES_H
