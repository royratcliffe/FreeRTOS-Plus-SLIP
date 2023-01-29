/*
 * slip.c
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

#include "slip.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "message_buffer.h"

#define END 0300U
#define ESC 0333U

#define ESC_END 0334U
#define ESC_ESC 0335U

static uint8_t ucEnd[] = {END};
static uint8_t ucEscEnd[] = {ESC, END};
static uint8_t ucEscEsc[] = {ESC, ESC};

typedef struct SLIP
{
	MessageBufferHandle_t hRxMessageBuffer;
	MessageBufferHandle_t hTxMessageBuffer;
	StreamBufferHandle_t hRxStreamBuffer;
	StreamBufferHandle_t hTxStreamBuffer;
	TaskHandle_t hRxTask;
	TaskHandle_t hTxTask;
} SLIP_t;

/*!
 * Receives a stream of bytes. Sends packets after SLIP protocol decoding.
 * Parameters include: a receiver queue handle for input and a message buffer
 * for output.
 */
static portTASK_FUNCTION(prvRxTask, pvParameters)
{
	SLIP_t *slip = pvParameters;
	uint8_t ucRxByte;
	uint8_t ucReceivedBytes[slipMAX_PACKET_LEN];
	size_t xReceivedBytes = 0UL;
	for (;;)
	{
		xStreamBufferReceive(slip->hRxStreamBuffer, &ucRxByte, sizeof(ucRxByte), portMAX_DELAY);
		switch (ucRxByte)
		{
		case END:
			if (xReceivedBytes > 0UL)
			{
				xMessageBufferSend(slip->hRxMessageBuffer, ucReceivedBytes, xReceivedBytes, portMAX_DELAY);
				xReceivedBytes = 0UL;
			}
			break;
		case ESC:
			xStreamBufferReceive(slip->hRxStreamBuffer, &ucRxByte, sizeof(ucRxByte), portMAX_DELAY);
			switch (ucRxByte)
			{
			case ESC_END:
				ucRxByte = END;
				break;
			case ESC_ESC:
				ucRxByte = ESC;
			}
		default:
			if (xReceivedBytes < slipMAX_PACKET_LEN)
			{
				ucReceivedBytes[xReceivedBytes++] = ucRxByte;
			}
		}
	}
}

/*!
 * Receives the entire next message dynamically.
 *
 * \param pucRxData Pointer to received data pointer typed as eight-bit unsigned
 * bytes.
 *
 * \returns Number of bytes received. The answer _could_ be zero if the message
 * buffer contains a zero-length message.
 *
 * The implementation relies on the all-or-nothing interface deployed by message
 * buffers: receive the entire message or read nothing. In this case, zero
 * received bytes indicates failure rather than a successful zero-length
 * message.
 */
static size_t prvMessageBufferReceive(MessageBufferHandle_t hMessageBuffer, uint8_t *pucRxData)
{
	size_t xReceivedBytes = xMessageBufferReceive(hMessageBuffer, pucRxData, 1UL, portMAX_DELAY);
	if (xReceivedBytes == 0UL)
	{
		size_t xLengthBytes = xMessageBufferNextLengthBytes(hMessageBuffer);
		xReceivedBytes = xMessageBufferReceive(hMessageBuffer, pucRxData, xLengthBytes, portMAX_DELAY);
	}
	return xReceivedBytes;
}

static portTASK_FUNCTION(prvTxTask, pvParameters)
{
	SLIP_t *slip = pvParameters;
	uint8_t ucTxData[slipMAX_PACKET_LEN];
	for (;;)
	{
		/*
		 * The name of the data is transmit data even though the transmit task
		 * _receives_ it. The term "transmit" here refers to the pipeline as a
		 * whole.
		 */
		size_t xReceivedBytes = prvMessageBufferReceive(slip->hTxMessageBuffer, ucTxData);
		xMessageBufferSend(slip->hTxStreamBuffer, ucEnd, sizeof(ucEnd), portMAX_DELAY);
		for (size_t x = 0UL; x < xReceivedBytes; x++)
		{
			switch (ucTxData[x])
			{
			case END:
				xMessageBufferSend(slip->hTxStreamBuffer, ucEscEnd, sizeof(ucEscEnd), portMAX_DELAY);
				break;
			case ESC:
				xMessageBufferSend(slip->hTxStreamBuffer, ucEscEsc, sizeof(ucEscEsc), portMAX_DELAY);
				break;
			default:
				xMessageBufferSend(slip->hTxStreamBuffer, ucTxData + x, sizeof(*ucTxData), portMAX_DELAY);
			}
		}
		xMessageBufferSend(slip->hTxStreamBuffer, ucEnd, sizeof(ucEnd), portMAX_DELAY);
	}
}

SLIPHandle_t xSLIPCreate(size_t xBufferSizeBytes, size_t xTriggerLevelBytes)
{
	SLIP_t *slip = pvPortMalloc(sizeof(SLIP_t));
	configASSERT(slip != NULL);
	slip->hRxMessageBuffer = xMessageBufferCreate(xBufferSizeBytes);
	slip->hTxMessageBuffer = xMessageBufferCreate(xBufferSizeBytes);
	slip->hRxStreamBuffer = xStreamBufferCreate(xBufferSizeBytes, xTriggerLevelBytes);
	slip->hTxStreamBuffer = xStreamBufferCreate(xBufferSizeBytes, xTriggerLevelBytes);
	xTaskCreate(prvRxTask, "SLIPRx", configMINIMAL_STACK_SIZE + slipMAX_PACKET_LEN, slip, slipRX_PRIORITY, &slip->hRxTask);
	xTaskCreate(prvTxTask, "SLIPTx", configMINIMAL_STACK_SIZE + slipMAX_PACKET_LEN, slip, slipTX_PRIORITY, &slip->hTxTask);
	return slip;
}

size_t xSLIPReceive(SLIPHandle_t xSLIP, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait)
{
	SLIP_t *slip = xSLIP;
	return xMessageBufferReceive(slip->hRxMessageBuffer, pvRxData, xBufferLengthBytes, xTicksToWait);
}

size_t xSLIPRxSend(SLIPHandle_t xSLIP, void *pvRxData, size_t xBufferLengthBytes, TickType_t xTicksToWait)
{
	SLIP_t *slip = xSLIP;
	return xStreamBufferSend(slip->hRxStreamBuffer, pvRxData, xBufferLengthBytes, xTicksToWait);
}

size_t xSLIPSend(SLIPHandle_t xSLIP, void *pvTxData, size_t xBufferLengthBytes, TickType_t xTicksToWait)
{
	if (xBufferLengthBytes == 0UL) return 0UL;
	SLIP_t *slip = xSLIP;
	if (xBufferLengthBytes > slipMAX_PACKET_LEN) xBufferLengthBytes = slipMAX_PACKET_LEN;
	return xMessageBufferSend(slip->hTxMessageBuffer, pvTxData, xBufferLengthBytes, xTicksToWait);
}

size_t xSLIPTxReceive(SLIPHandle_t xSLIP, void *pvTxData, size_t xBufferLengthBytes, TickType_t xTicksToWait)
{
	SLIP_t *slip = xSLIP;
	return xStreamBufferReceive(slip->hTxStreamBuffer, pvTxData, xBufferLengthBytes, xTicksToWait);
}
