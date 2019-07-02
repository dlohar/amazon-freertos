/*
 * Amazon FreeRTOS HTTPS Client V1.0.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file iot_https_client.h
 * @brief Implementation of the user-facing functions of the Amazon FreeRTOS HTTPS Client library. 
 */

/* The config header is always included first. */
#include "iot_config.h"

/* Error handling include. */
#include "private/iot_error.h"

/* HTTPS Client library private inclues. */
#include "private/iot_https_internal.h"


static IotHttpsReturnCode_t _sendHttpsHeaders( _httpsConnection_t* _httpsConnection, uint8_t* pHeadersBuf, uint32_t headersLength, bool isNonPersistent, uint32_t contentLength);


static IotHttpsReturnCode_t _sendHttpsHeaders( _httpsConnection_t* _httpsConnection, uint8_t* pHeadersBuf, uint32_t headersLength, bool isNonPersistent, uint32_t contentLength)
{
    const char* connectionHeader = NULL;
    IotHttpsReturnCode_t status = IOT_HTTPS_OK;
    int numWritten = 0;
    int connectionHeaderLen = 0;
    char contentLengthHeaderStr[27];
    char finalHeaders[52] = { 0 };
    if (isNonPersistent)
    {
        connectionHeader = "Connection: closed\r\n";
        connectionHeaderLen = strlen("Connection: closed\r\n");
    }
    else
    {
        connectionHeader = "Connection: keep-alive\r\n";
        connectionHeaderLen = strlen("Connection: keep-alive\r\n");
    }
    size_t length = strlen(connectionHeader);
    memcpy(&finalHeaders[numWritten], connectionHeader, connectionHeaderLen);
    return status;
}

IotHttpsReturnCode_t IotHttpsClient_SendSync(IotHttpsConnectionHandle_t *pConnHandle, IotHttpsRequestHandle_t reqHandle, IotHttpsResponseHandle_t * pRespHandle)
{

    _httpsConnection_t * _httpsConnection = NULL;
    IotHttpsReturnCode_t status;
    /* Set the internal response context to use and the response handle to return. */
    /* Set the internal connection context since we are connected now. */
    _httpsConnection = *pConnHandle;
    status = _sendHttpsHeaders( _httpsConnection,
        reqHandle->pHeaders,
        reqHandle->pHeadersCur - reqHandle->pHeaders,
        reqHandle->isNonPersistent,
        reqHandle->bodyLength);
}
