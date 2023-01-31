/*
 * message_buffer_ex.c
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

#include "message_buffer_ex.h"

#include "task.h"

size_t xMessageBufferReceiveMalloc(MessageBufferHandle_t xMessageBuffer, void **ppvRxData, TickType_t xTicksToWait)
{
	configASSERT(xMessageBuffer != NULL);
	configASSERT(ppvRxData != NULL);
	void *pvRxData;
	uint8_t ucRxByte;
	size_t xReceivedBytes = xMessageBufferReceive(xMessageBuffer, &ucRxByte, sizeof(ucRxByte), xTicksToWait);
	if (xReceivedBytes == 0UL)
	{
		size_t xLengthBytes = xMessageBufferNextLengthBytes(xMessageBuffer);
		pvRxData = pvPortMalloc(xLengthBytes);
		if (pvRxData == NULL)
		{
			*ppvRxData = NULL;
			return 0UL;
		}
		xReceivedBytes = xMessageBufferReceive(xMessageBuffer, pvRxData, xLengthBytes, xTicksToWait);
	}
	else
	{
		pvRxData = pvPortMalloc(xReceivedBytes);
		if (pvRxData == NULL)
		{
			*ppvRxData = NULL;
			return 0UL;
		}
		*(uint8_t *)pvRxData = ucRxByte;
	}
	*ppvRxData = pvRxData;
	return xReceivedBytes;
}
