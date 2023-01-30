/*
 * slip.h
 * Copyright (c) 2023, Roy Ratcliffe, Northumberland, United Kingdom
 *
 * Permission is hereby granted, free of charge,  to any person obtaining a
 * copy  of  this  software  and    associated   documentation  files  (the
 * "Software"), to deal in  the   Software  without  restriction, including
 * without limitation the rights to  use,   copy,  modify,  merge, publish,
 * distribute, sublicense, and/or sell  copies  of   the  Software,  and to
 * permit persons to whom the Software is   furnished  to do so, subject to
 * the following conditions:
 *
 *     The above copyright notice and this permission notice shall be
 *     included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT  WARRANTY OF ANY KIND, EXPRESS
 * OR  IMPLIED,  INCLUDING  BUT  NOT   LIMITED    TO   THE   WARRANTIES  OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR   PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS  OR   COPYRIGHT  HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY,  WHETHER   IN  AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM,  OUT  OF   OR  IN  CONNECTION  WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "FreeRTOS.h"

#ifndef slipMAX_PACKET_LEN
#define slipMAX_PACKET_LEN 255
#endif

#ifndef slipRX_PRIORITY
#define slipRX_PRIORITY (tskIDLE_PRIORITY)
#endif

#ifndef slipTX_PRIORITY
#define slipTX_PRIORITY (tskIDLE_PRIORITY)
#endif

typedef void *SLIPHandle_t;

/*!
 * Creates a new SLIP infrastructure wrapper dynamically.
 *
 * Creates a new receiver and transmitter task with the necessary stack sizes,
 * amounting to minimal size plus the maximum packet length. Reserve sufficient
 * heap space for the new tasks and their stacks.
 */
SLIPHandle_t xSLIPCreate(size_t xBufferSizeBytes, size_t xTriggerLevelBytes);

/*!
 * Receives a packet.
 */
size_t xSLIPReceive(SLIPHandle_t xSLIP, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait);

size_t xSLIPReceiveMalloc(SLIPHandle_t xSLIP, void **ppvRxData, TickType_t xTicksToWait);

/*!
 * Fills the reception stream.
 */
size_t xSLIPRxSend(SLIPHandle_t xSLIP, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait);

/*!
 * Sends a packet.
 *
 * Filters out zero-length packets. Sending no bytes amounts to a no-operation;
 * it always returns immediately.
 *
 * Automatically trims the packet if its length exceeds the maximum length.
 */
size_t xSLIPSend(SLIPHandle_t xSLIP, void *pvTxData, size_t xBufferLengthBytes, TickType_t xTicksToWait);

/*!
 * Empties the transmission stream.
 *
 * Receives transmission data from the transmitter stream.
 */
size_t xSLIPTxReceive(SLIPHandle_t xSLIP, void *pvTxData, size_t xBufferLengthBytes, TickType_t xTicksToWait);
