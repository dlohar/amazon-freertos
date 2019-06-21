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

/*-----------------------------------------------------------*/

/**
 * @brief The length of a request line for the requestUserBufferMinimumSize calculation.
 * 
 * This is the length of "CONNECT / HTTP/1.1\r\n". The minimum path is "/" because we cannot know how long the 
 * application requested path is is going to be. CONNECT is the longest string length HTTP method according to RFC 2616.
 */
#define HTTPS_REQUEST_LINE_LENGTH                   20

/**
 * @brief The HTTP protocol version of this library is HTTP/1.1.
 */
#define HTTPS_PROTOCOL_VERSION                      "HTTP/1.1"

/*
 * Constants for header fields added automatically in the request initialization.
 */
#define HTTPS_USER_AGENT_HEADER                     "User-Agent"
#define HTTPS_HOST_HEADER                           "Host"

/*
 * Constants for the header fields added automatically during the sending of the HTTP request.
 */
#define HTTPS_CONTENT_LENGTH_HEADER                 "Content-Length"
#define HTTPS_CONNECTION_HEADER                     "Connection"

/*
 * Constants for the values of the HTTP "Connection" header field.
 * 
 * This is used for writing headers automatically in during the sending of the HTTP request.
 * "Connection: keep-alive\r\n" is written automatically for a persistent connection.
 * "Connection: close\r\n" is written automatically for a closed connection.
 */
#define HTTPS_CONNECTION_KEEP_ALIVE_HEADER_VALUE    "keep-alive"
#define HTTPS_CONNECTION_CLOSE_HEADER_VALUE         "closed"

/**
 * Constants for HTTP header formatting. 
 * 
 * ": " separates and header field from the header value.
 * "\r\n" Ends the header line.
 */
#define HTTPS_HEADER_FIELD_SEPARATOR                ": "
#define HTTPS_END_OF_HEADER_LINES_INDICATOR         "\r\n"

/**
 * @brief The string length of the maximum file size that can be represented in a 32 bit system.
 * 
 * If a number in a 32 bit system cannot be more than 32 bits, then 2^32 = 4294967296 which is 10 digits in length.
 */
#define HTTPS_MAX_FILE_SIZE_LENGTH                  10      /* strlen(str(2^32)) */

/**
 * @brief The User-Agent header line string.
 * 
 * This is of the form:
 * "User-Agent: <configured-user-agent>\r\n"
 * This is used for the calculation of the requestUserBufferMinimumSize.
 */
#define HTTPS_USER_AGENT_HEADER_LINE            HTTPS_USER_AGENT_HEADER HTTPS_HEADER_FIELD_SEPARATOR IOT_HTTPS_USER_AGENT HTTPS_END_OF_HEADER_LINES_INDICATOR

/**
 * @brief The Host header line with the field only and not the value.
 * 
 * This is of the form:
 * "Host: \r\n"
 * This is used for the calculation of the requestUserBufferMinimumSize. The Host value is not specified because we
 * cannot anticipate what server the client is making requests to.
 */
#define HTTPS_PARTIAL_HOST_HEADER_LINE          HTTPS_HOST_HEADER HTTPS_HEADER_FIELD_SEPARATOR HTTPS_END_OF_HEADER_LINES_INDICATOR   

/**
 * @brief The maximum Content-Length header line size.
 * 
 * This is the length of header line string: "Content-Length: 4294967296\r\n". 4294967296 is 2^32. This number is chosen
 * because it is the maximum file size that can be represented in a 32 bit system.
 */
#define HTTPS_MAX_CONTENT_LENGTH_LINE_LENGTH    26

/*
 * String constants for the Connection header and possible values.
 * 
 * This is used for writing headers automatically in during the sending of the HTTP request.
 * "Connection: keep-alive\r\n" is written automatically for a persistent connection.
 * "Connection: close\r\n" is written automatically for a closed connection.
 */
#define HTTPS_CONNECTION_KEEP_ALIVE_HEADER_LINE HTTPS_CONNECTION_HEADER HTTPS_HEADER_FIELD_SEPARATOR HTTPS_CONNECTION_KEEP_ALIVE_HEADER_VALUE HTTPS_END_OF_HEADER_LINES_INDICATOR
#define HTTPS_CONNECTION_CLOSE_HEADER_LINE      HTTPS_CONNECTION_HEADER HTTPS_HEADER_FIELD_SEPARATOR HTTPS_CONNECTION_CLOSE_HEADER_VALUE HTTPS_END_OF_HEADER_LINES_INDICATOR

/**
 * @brief The length of the "Connection: keep-alive\r\n" header. 
 * 
 * This is used for sizing a local buffer for the final headers to send that include the "Connection: keep-alive\r\n" 
 * header line. 
 */
#define HTTPS_CONNECTION_KEEP_ALIVE_HEADER_LINE_LENGTH  24

/*-----------------------------------------------------------*/

/**
 * @brief Minimum size of the request user buffer.
 * 
 * The user buffer is configured in IotHttpsClientRequestInfo_t.reqUserBuffer. This buffer stores the internal context
 * of the request then the request headers right after. The minimum size for the buffer is the total size of the 
 * internal request context, the HTTP formatted request line, the User-Agent header line, and the part of the Host 
 * header line.
 */
const uint32_t requestUserBufferMinimumSize = sizeof(_httpsRequest_t) + 
                HTTPS_REQUEST_LINE_LENGTH +
                sizeof(HTTPS_USER_AGENT_HEADER_LINE) +
                sizeof(HTTPS_PARTIAL_HOST_HEADER_LINE);

/**
 * @brief Minimum size of the response user buffer.
 * 
 * The user buffer is configured in IotHttpsClientRequestInfo_t.respUserBuffer. This buffer stores the internal context
 * of the response and then the response headers right after. 
 */
const uint32_t responseUserBufferMinimumSize = sizeof(_httpsRequest_t);

/**
 * @brief Minimum size of the connection user buffer.
 * 
 * The user buffer is configured in IotHttpsConnectionInfo_t.userBuffer. This buffer stores the internal context of the 
 * connection.
 */
const uint32_t connectionUserBufferMinimumSize = sizeof(_httpsConnection_t);

/*-----------------------------------------------------------*/

/**
 * @brief Callback for http-parser to indicate the start of the HTTP response message is reached.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_message_begin.
 */
static int _httpParserOnMessageBeginCallback(http_parser * pHttpParser);

/**
 * @brief Callback for http-parser to indicate it found the HTTP response status code.
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * @param[in] pLoc - Pointer to the HTTP response status code string in the response message buffer.
 * @param[in] length - The length of the HTTP response status code string.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_status.
 */
static int _httpParserOnStatusCallback(http_parser * pHttpParser, const char * pLoc, size_t length);

/**
 * @brief Callback for http-parser to indicate it found an HTTP response header field.
 * 
 * If only part of the header field was returned here in this callback, then this callback will be invoked again the
 * next time the parser executes on the next part of the header field.
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * @param[in] pLoc - Pointer to the header field string in the response message buffer.
 * @param[in] length - The length of the header field.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_header_field.
 */
static int _httpParserOnHeaderFieldCallback(http_parser * pHttpParser, const char * pLoc, size_t length);

/**
 * @brief Callback for http-parser to indicate it found an HTTP response header value.
 * 
 * This value corresponds to the field that was found in the _httpParserOnHeaderFieldCallback() called immediately 
 * before this callback was called.
 * 
 * If only part of the header value was returned here in this callback, then this callback will be invoked again the
 * next time the parser executes on the next part of the header value.
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * @param[in] pLoc - Pointer to the header value string in the response message buffer.
 * @param[in] length - The length of the header value.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_header_value.
 */
static int _httpParserOnHeaderValueCallback(http_parser * pHttpParser, const char * pLoc, size_t length);

/**
 * @brief Callback for http-parser to indicate it reached the end of the headers in the HTTP response messsage.
 * 
 * The end of the headers is signalled in a HTTP response message by another "\r\n" after the final header line.
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_headers_complete.
 */
static int _httpParserOnHeadersCompleteCallback(http_parser * pHttpParser);

/**
 * @brief Callback for http-parser to indicate it found HTTP response body.
 * 
 * This callback will be invoked multiple times if there response body is of "Transfer-Encoding: chunked". 
 * _httpParserOnChunkHeaderCallback() will be invoked first, then _httpParserOnBodyCallback(), then 
 * _httpParserOnChunkCompleteCallback(), then repeated back to _httpParserOnChunkHeaderCallback() if there are more
 * "chunks".
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * @param[in] pLoc - Pointer to the body string in the response message buffer.
 * @param[in] length - The length of the body found.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_body.
 */
static int _httpParserOnBodyCallback(http_parser * pHttpParser, const char * pLoc, size_t length);

/**
 * @brief Callback for http-parser to indicate it reached the end of the HTTP response messsage.
 * 
 * The end of the message is signalled in a HTTP response message by another "\r\n" after the final header line, with no
 * entity body; or it is signally by "\r\n" at the end of the entity body.
 * 
 * For a Transfer-Encoding: chunked type of response message, the end of the message is signalled by a terminating 
 * chunk header with length zero.
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_message_complete.
 */
static int _httpParserOnMessageCompleteCallback(http_parser * pHttpParser);

/* To save code space we do not compile in this code that just prints debugging. */
#if (LIBRARY_LOG_LEVEL == IOT_LOG_DEBUG)
/**
 * @brief Callback for http-parser to indicate it found an HTTP Transfer-Encoding: chunked header.
 * 
 * Transfer-Encoding: chunked headers are embedded in the HTTP response entity body by a "\r\n" followed by the size of
 * the chunk followed by another "\r\n".
 * 
 * If only part of the header field was returned here in this callback, then this callback will be invoked again the
 * next time the parser executes on the next part of the header field.
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_chunk_header.
 */
static int _httpParserOnChunkHeaderCallback(http_parser * pHttpParser);

/**
 * @brief Callback for http-parser to indicate it reached the end of an HTTP response messsage "chunk".
 * 
 * A chunk is complete when the chunk header size is read fully in the body.
 * 
 * See https://github.com/nodejs/http-parser for more information.
 * 
 * @param[in] pHttpParser - http-parser state structure.
 * 
 * @return 0 to tell http-parser to keep parsing.
 *         1 to tell http-parser than parsing should stop return from http_parser_execute with error HPE_CB_chunk_complete.
 */
static int _httpParserOnChunkCompleteCallback(http_parser * pHttpParser);
#endif

/**
 * @brief Network receive callback for the HTTPS Client library.
 * 
 * This function is called by the network abstraction whenever data is available for the HTTP library.
 * 
 * @param[in] pNetworkConnection - The network connection with the HTTPS connection, pass by the network stack.
 * @param[in] pReceiveContext - A pointer to the HTTPS Client connection handle for which the packet was received.
 */
static void _networkReceiveCallback( void* pNetworkConnection, void* pReceiveContext );

/**
 * @brief Connects to HTTPS server and initializes the connection context.
 * 
 * @param[out] pConnHandle - Handle returned representing the open connection.
 * @param[in] pConnConfig - The connection configuration.
 * 
 * @return #IOT_HTTPS_OK if the connection was successful and so was initializing the context.
 *         #IOT_HTTPS_CONNECTION_ERROR if the connection failed.
 *         #IOT_HTTPS_INTERNAL_ERROR if the context failed to initialize.
 */
static IotHttpsReturnCode_t _connectHttpsServer(IotHttpsConnectionHandle_t * pConnHandle, IotHttpsConnectionInfo_t *pConnConfig);

/**
 * @brief Disconnects from the network.
 * 
 * @param[in] _httpsConnection - HTTPS connection handle.
 * 
 * @return #IOT_HTTPS_OK of the disconnect was successful.
 *         #IOT_HTTPS_NETWORK_ERROR if the disconnect failed.
 */
static IotHttpsReturnCode_t _networkDisconnect(_httpsConnection_t* _httpsConnection);

/**
 * @brief Add a header to the current HTTP request.
 * 
 * The headers are stored in reqHandle->pHeaders.
 * 
 * @param[in] reqHandle - HTTP request context.
 * @param[in] pName - The name of the header to add. This is a NULL terminated string.
 * @param[in] pValue - The buffer to value string.
 * @param[in] len - The length of the header value string.
 * 
 * @return #IOT_HTTPS_OK if the header was added to the request successfully.
 *         #IOT_HTTPS_INSUFFICIENT_MEMORY if there was not room in the IotHttpsRequestHandle_t->pHeaders.
 *         #IOT_HTTPS_INTERNAL_ERROR if snprintf() failed to write to IotHttpsRequestHandle_t->pHeadersCur.
 */
static IotHttpsReturnCode_t _addHeader(IotHttpsRequestHandle_t reqHandle, const char * pName, const char * pValue, uint32_t len );

/**
 * @brief Send data on the network.
 * 
 * @param[in] _httpsConnection - HTTP connection context.
 * @param[in] pBuf - Buffer of data to send.
 * @param[in] len - The length of the data to send.
 * 
 * @return #IOT_HTTPS_OK if the data sent successfully.
 *         #IOT_HTTPS_NETWORK_ERROR if there was an error sending the data on the network.
 */
static IotHttpsReturnCode_t _networkSend(_httpsConnection_t* _httpsConnection, uint8_t * pBuf, size_t len);

/**
 * @brief Receive data on the network.
 * 
 * @param[in] _httpsConnection - HTTP connection context.
 * @param[in] pBuf - Buffer of data to receive into.
 * @param[in] len - The length of the data to receive.
 * 
 * @return #IOT_HTTPS_OK if the data was received successfully.
 *         #IOT_HTTPS_NETWORK_ERROR if there was an error receiving the data on the network.
 *         #IOT_HTTPS_TIMEOUT_ERROR if we timedout trying to receive data from the network.
 */
static IotHttpsReturnCode_t _networkRecv( _httpsConnection_t* _httpsConnection, uint8_t * pBuf, size_t bufLen);

/**
 * @brief Send all of the HTTP request headers in the pHeadersBuf and the final Content-Length and Connection headers.
 * 
 * All of the headers in headerbuf are sent first followed by the computed content length and persistent connection 
 * indication.
 * 
 * @param[in] _httpsConnection - HTTP connection context.
 * @param[in] pHeadersBuf - Buffer of the request headers to send.
 * @param[in] headersLength - The length of the request headers to send.
 * @param[in] isNonPersistent - Indicator of whether the connection is persistent or not.
 * @param[in] contentLength - The length of the request body.
 * 
 * @return #IOT_HTTPS_OK if the headers were fully sent successfully.
 *         #IOT_HTTPS_NETWORK_ERROR if there was an error receiving the data on the network.
 *         #IOT_HTTPS_TIMEOUT_ERROR if we timedout trying to receive data from the network.
 */
static IotHttpsReturnCode_t _sendHttpsHeaders( _httpsConnection_t* _httpsConnection, uint8_t* pHeadersBuf, uint32_t headersLength, bool isNonPersistent, uint32_t contentLength);

/**
 * @brief Send all of the HTTP request body in pBodyBuf.
 * 
 * @param[in] _httpsConnection - HTTP connection context.
 * @param[in] pBodyBuf - Buffer of the request body to send.
 * @param[in] bodyLength - The length of the body to send.
 * 
 * @return #IOT_HTTPS_OK if the body was fully sent successfully.
 *         #IOT_HTTPS_NETWORK_ERROR if there was an error receiving the data on the network.
 *         #IOT_HTTPS_TIMEOUT_ERROR if we timedout trying to receive data from the network.
 */
static IotHttpsReturnCode_t _sendHttpsBody( _httpsConnection_t* _httpsConnection, uint8_t* pBodyBuf, uint32_t bodyLength);

/**
 * @brief Parse the HTTP response message in pBuf.
 * 
 * @param[in] pHttpParser - Pointer to the instance of the http-parser.
 * @param[in] pBuf - The buffer of data to parse.
 * @param[in] len - The length of data to parse.
 * 
 * @return #IOT_HTTPS_OK if the data was parsed successfully.
 *         #IOT_HTTPS_PARSING_ERROR if there was an error with parsing the data.
 */
static IotHttpsReturnCode_t _parseHttpsMessage(http_parser* pHttpParser, char* pBuf, size_t len);

/**
 * @brief Receive any part of an HTTP response.
 * 
 * This function is used for both receiving the body into the body buffer and receiving the header into the header 
 * buffer.
 * 
 * @param[in] _httpsConnection - HTTP Connection context.
 * @param[in] pParser - Pointer to the instance of the http-parser.
 * @param[in] pCurrentParserState - The current state of what has been parsed in the HTTP response.
 * @param[in] finalParserState - The final state of the parser expected after this function finishes.
 * @param[in] pBuf - Pointer to the buffer to receive the HTTP response into.
 * @param[in] pBufCur - Pointer to the next location to write data into the buffer pBuf. This is double pointer to update the response context buffer pointers.
 * @param[in] pBufEnd - Pointer to the end of the buffer to receive the HTTP response into.
 * @param[out] pNetworkStatus - The network status will be returned here. The network status can be any of the return values from _networkRecv().
 * 
 * @return #IOT_HTTPS_OK if we received the HTTP response message part successfully.
 *         #IOT_HTTPS_PARSING_ERROR if there was an error with parsing the data.
 */
static IotHttpsReturnCode_t _receiveHttpsMessage( _httpsConnection_t* _httpsConnection, 
                                                  http_parser* pParser,
                                                  IotHttpsResponseParserState_t *pCurrentParserState,
                                                  IotHttpsResponseParserState_t finalParserState, 
                                                  uint8_t** pBuf,
                                                  uint8_t** pBufCur,
                                                  uint8_t** pBufEnd,
                                                  IotHttpsReturnCode_t *pNetworkStatus);

/**
 * @brief Receive the HTTP response headers.
 * 
 * This function will set the Content-Length in the response context if it is found in the headers received.
 * Receiving the response headers is always the first step in receiving the response, therefore the 
 * _httpsResponse->httpParser will be initialized to a starting state when this function is called.
 * 
 * This function sets internal states to indicate that the header buffer is being processed now for a new response.
 * 
 * @param[in] _httpsConnection - HTTP connection context.
 * @param[in] _httpsResponse - HTTP response context.
 * @param[out] pNetworkStatus - The network status will be returned here. The network status can be any of the return values from _networkRecv().
 * 
 * @return #IOT_HTTPS_OK if we received the HTTP headers successfully.
 *         #IOT_HTTPS_PARSING_ERROR if there was an error with parsing the header buffer.
 */
static IotHttpsReturnCode_t _receiveHttpsHeaders( _httpsConnection_t* _httpsConnection, _httpsResponse_t* _httpsResponse, IotHttpsReturnCode_t *pNetworkStatus);

/**
 * @brief Receive the HTTP response body.
 * 
 * Sets internal states to indicate that the the body buffer is being processed now for a new response.
 * 
 * @param[in] _httpsConnection - HTTP connection context.
 * @param[in] _httpsResponse - HTTP response context.
 * @param[out] pNetworkStatus - The network status will be returned here. The network status can be any of the return values from _networkRecv().
 * 
 * @return #IOT_HTTPS_OK if we received the HTTP body successfully.
 *         #IOT_HTTPS_PARSING_ERROR if there was an error with parsing the body buffer.
 */
static IotHttpsReturnCode_t _receiveHttpsBody( _httpsConnection_t* _httpsConnection, _httpsResponse_t* _httpsResponse, IotHttpsReturnCode_t *pNetworkStatus);

/**
 * @brief Read the rest of any HTTP response that may be on the network.
 * 
 * This reads the rest of any left over response data that might still be on the network buffers. We do not want this
 * data left over because it will spill into the header and body buffers of next response that we try to receive. 
 * 
 * If we performed a request without a body and the headers received exceeds the size of the 
 * _httpResponse->pHeaders buffer, then we need to flush the network buffer.
 * 
 * If the application configured the body buffer as null in IotHttpsResponseInfo_t.syncInfo.respData and the server 
 * sends body in the response, but it exceeds the size of  _httpResponse->pHeaders buffer, then we need to flush the 
 * network buffer.
 * 
 * If the amount of body received on the network does not fit into a non-null IotHttpsResponseInfo_t.syncInfo.respData, 
 * then we need to flush the network buffer.
 * 
 * If an asynchronous request cancels in the middle of a response process, after already sending the request message, 
 * then we need to flush the network buffer.
 * 
 * @param[in] _httpsConnection - HTTP connection context.
 * @param[in] _httpsResponse - HTTP response context.
 * 
 * @return #IOT_HTTPS_OK if we received successfully flushed the network data.
 *         #IOT_HTTPS_PARSING_ERROR if there was an error with parsing the data.
 *         #IOT_HTTPS_NETWORK_ERROR if there was an error receiving the data on the network.
 *         #IOT_HTTPS_TIMEOUT_ERROR if we timedout trying to receive data from the network.
 */
static IotHttpsReturnCode_t _flushHttpsNetworkData( _httpsConnection_t* _httpsConnection, _httpsResponse_t* _httpsResponse );

/*-----------------------------------------------------------*/

/**
 * @brief A map of the method enum to strings 
 * 
 * These are in order to the HTTP request method enums defined in IotHttpsMethod_t.
 */
static const char* httpsMethodStrings[] = {
    "GET",
    "HEAD"
};

/**
 * @brief Definition of the http-parser settings.
 * 
 * The http_parser_settings holds all of the callbacks invoked by the http-parser.
 */
static http_parser_settings _httpParserSettings = { 0 };

/*-----------------------------------------------------------*/

static int _httpParserOnMessageBeginCallback(http_parser * pHttpParser)
{
    IotLogDebug("Parser: Start of HTTPS Response message.");
    _httpsResponse_t *_httpsResponse = (_httpsResponse_t*)(pHttpParser->data);
    /* Set the state of the parser. The headers are at the start of the message always. */
    _httpsResponse->parserState = PARSER_STATE_IN_HEADERS;
    return 0;
}

/*-----------------------------------------------------------*/

static int _httpParserOnStatusCallback(http_parser * pHttpParser, const char * pLoc, size_t length)
{
    _httpsResponse_t *_httpsResponse = (_httpsResponse_t*)(pHttpParser->data);
    IotLogDebug("Parser: Status %.*s retrieved from HTTPS response.", length, pLoc);
    /* Save the status code so it can be retrieved with IotHttpsClient_ReadResponseStatus(). */
    _httpsResponse->status = pHttpParser->status_code;
    /* If we are parsing the network data received in the header buffer then we can increment 
      _httpsResponse->pHeadersCur. The status line in the response is part of the data stored in
      _httpResponse->pHeaders. */
    if(_httpsResponse->bufferProcessingState == PROCESSING_STATE_FILLING_HEADER_BUFFER)
    {
        _httpsResponse->pHeadersCur = (uint8_t*)(pLoc += length);
    }
    return 0;
}

/*-----------------------------------------------------------*/

static int _httpParserOnHeaderFieldCallback(http_parser * pHttpParser, const char * pLoc, size_t length)
{
    IotLogDebug("Parser: HTTPS header field parsed %.*s", length, pLoc);
    _httpsResponse_t * _httpsResponse = (_httpsResponse_t *)(pHttpParser->data);
    /* If we are parsing the network data received in the header buffer then we can increment 
      _httpsResponse->pHeadersCur. */
    if(_httpsResponse->bufferProcessingState == PROCESSING_STATE_FILLING_HEADER_BUFFER)
    {
        _httpsResponse->pHeadersCur = (uint8_t*)(pLoc += length);
    }
    /* If the IotHttpsClient_ReadHeader() was called, then we check for the header field of interest. */
    if(_httpsResponse->bufferProcessingState == PROCESSING_STATE_SEARCHING_HEADER_BUFFER)
    {
        if(strncmp(_httpsResponse->pReadHeaderField, pLoc, length) == 0)
        {
            _httpsResponse->foundHeaderField = true;   
        }
    }
    return 0;
}

/*-----------------------------------------------------------*/

static int _httpParserOnHeaderValueCallback(http_parser * pHttpParser, const char * pLoc, size_t length)
{
    IotLogDebug("Parser: HTTPS header value parsed %.*s", length, pLoc);
    _httpsResponse_t * _httpsResponse = (_httpsResponse_t *)(pHttpParser->data);
    /* If we are parsing the network data received in the header buffer then we can increment 
      _httpsResponse->pHeadersCur. */
    if(_httpsResponse->bufferProcessingState == PROCESSING_STATE_FILLING_HEADER_BUFFER)
    {
        _httpsResponse->pHeadersCur = (uint8_t*)(pLoc += length);
    }

    /* If the IotHttpsClient_ReadHeader() was called, then we check if we found the header field of interest. */
    if(_httpsResponse->bufferProcessingState == PROCESSING_STATE_SEARCHING_HEADER_BUFFER)
    {
        if(_httpsResponse->foundHeaderField)
        {
            _httpsResponse->pReadHeaderValue = ( uint8_t* )( pLoc );
            _httpsResponse->readHeaderValueLength = length;
            /* We found a header field so we don't want to keep parsing.*/
            return 1;
        }
    }
    return 0;
}

/*-----------------------------------------------------------*/

static int _httpParserOnHeadersCompleteCallback(http_parser * pHttpParser)
{
    IotLogDebug("Parser: End of the headers reached in the pHttpParser.");
    _httpsResponse_t * _httpsResponse = (_httpsResponse_t *)(pHttpParser->data);
    _httpsResponse->parserState = PARSER_STATE_HEADERS_COMPLETE;

    /* If the IotHttpsClient_ReadHeader() was called, we return after finishing looking through all of the headers. 
       Returning a non-zero value exists the http parsing. */
    if(_httpsResponse->bufferProcessingState == PROCESSING_STATE_SEARCHING_HEADER_BUFFER)
    {
        return 1;
    }

    /* When in this callback the pheaderCur pointer is at the first "\r" in the last header line. HTTP/1.1
       headers end with another "\r\n" at the end of the last line. This means we must increment
       the headerCur pointer to the length of "\r\n\r\n". */
    if(_httpsResponse->bufferProcessingState == PROCESSING_STATE_FILLING_HEADER_BUFFER)
    {
        _httpsResponse->pHeadersCur += strlen("\r\n\r\n");
    }

    /* content_length will be zero if no Content-Length header found by the parser. */
    _httpsResponse->contentLength = pHttpParser->content_length;

    /* For a HEAD method, there is no body expected in the response, so we return 1 to skip body parsing. 
       Also if it was configured in a synchronous response to ignore the HTTPS response body then also stop the body 
       parsing. */
    if((_httpsResponse->method == IOT_HTTPS_METHOD_HEAD) || (_httpsResponse->pBody == NULL))
    {
        return 1;
    }
    return 0;
}

/*-----------------------------------------------------------*/

static int _httpParserOnBodyCallback(http_parser * pHttpParser, const char * pLoc, size_t length)
{
    IotLogDebug("Parser: Reached the HTTPS message body. It is of length: %d", length);

    _httpsResponse_t * _httpsResponse = (_httpsResponse_t *)(pHttpParser->data);
    _httpsResponse->parserState = PARSER_STATE_IN_BODY;

    /* If we are in the header buffer and we come across HTTPS body, then we want to copy it to the body
       buffer. */
    /* Only copy the data if the current location is not the bodyCur. */
    if((_httpsResponse->pBodyCur != NULL) && (_httpsResponse->pBodyEnd - _httpsResponse->pBodyCur > 0))
    {
        if(_httpsResponse->pBodyCur != pLoc)
        {
            memcpy(_httpsResponse->pBodyCur, pLoc, length);
        }
        _httpsResponse->pBodyCur += length;
    }
    return 0;
}

/*-----------------------------------------------------------*/

static int _httpParserOnMessageCompleteCallback(http_parser * pHttpParser)
{
    IotLogDebug("Parser: End of the HTTPS message reached.");
    _httpsResponse_t * _httpsResponse = (_httpsResponse_t *)(pHttpParser->data);
    _httpsResponse->parserState = PARSER_STATE_BODY_COMPLETE;

    /* When this callback is reached the end of the HTTP message is indicated. If the length of buffer passed in is 
       larger than the message, then http_parser_execute will return either HPE_CLOSED_CONNECTION or 
       HPE_INVALID_CONSTANT. */
    return 0;
}

/*-----------------------------------------------------------*/

/* To save code space we do not compile in this code that just prints debugging. */
#if (LIBRARY_LOG_LEVEL == IOT_LOG_DEBUG)
static int _httpParserOnChunkHeaderCallback(http_parser * pHttpParser)
{
    IotLogDebug("Parser: HTTPS message Chunked encoding header callback.");
    IotLogDebug( "Parser: HTTPS message Chunk size: %d", pHttpParser->content_length );
    return 0;
}

/*-----------------------------------------------------------*/

static int _httpParserOnChunkCompleteCallback(http_parser * pHttpParser)
{
    IotLogDebug("End of a HTTPS message Chunk complete callback.");
    return 0;
}
#endif

/*-----------------------------------------------------------*/

static void _networkReceiveCallback( void* pNetworkConnection, void* pReceiveContext )
{
    _httpsConnection_t* _httpsConnection = ( _httpsConnection_t* )pReceiveContext;
    /* Post that any thread in the connection can read on the socket. */
    IotSemaphore_Post( &( _httpsConnection->rxStartSem ) );
    /* Block while that thread reads from the socket. */
    IotSemaphore_Wait( &( _httpsConnection->rxFinishSem ) );
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_Init( void )
{
    /* This sets all member in the _httpParserSettings to zero. It does not return any errors. */
    http_parser_settings_init( &_httpParserSettings );

    /* Set the http-parser callbacks. */
    _httpParserSettings.on_message_begin = _httpParserOnMessageBeginCallback;
    _httpParserSettings.on_status = _httpParserOnStatusCallback;
    _httpParserSettings.on_header_field = _httpParserOnHeaderFieldCallback;
    _httpParserSettings.on_header_value = _httpParserOnHeaderValueCallback;
    _httpParserSettings.on_headers_complete = _httpParserOnHeadersCompleteCallback;
    _httpParserSettings.on_body = _httpParserOnBodyCallback;
    _httpParserSettings.on_message_complete = _httpParserOnMessageCompleteCallback;
/* To save code space we do not compile in code that just prints debugging. */
#if (LIBRARY_LOG_LEVEL == IOT_LOG_DEBUG)
    _httpParserSettings.on_chunk_header = _httpParserOnChunkHeaderCallback;
    _httpParserSettings.on_chunk_complete = _httpParserOnChunkCompleteCallback;
#endif

    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

void IotHttpsClient_Deinit( void )
{
    /* The library has not taken any resources that need freeing. This implementation is here for formality.*/
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _connectHttpsServer(IotHttpsConnectionHandle_t * pConnHandle, IotHttpsConnectionInfo_t *pConnConfig)
{
    IOT_FUNCTION_ENTRY(IotHttpsReturnCode_t, IOT_HTTPS_OK);

    IotNetworkError_t networkStatus = IOT_NETWORK_SUCCESS;
    char pAlpnProtos[IOT_HTTPS_MAX_ALPN_PROTOCOLS_LENGTH] = { 0 };
    char pHostName[IOT_HTTPS_MAX_HOST_NAME_LENGTH] = { 0 };
    bool connSemCreated = false;
    bool rxStartSemCreated = false;
    bool rxFinishedSemCreated = false;
    IotNetworkServerInfo_t networkServerInfo = { 0 };
    IotNetworkCredentials_t networkCredentials = { 0 };
    _httpsConnection_t *_httpsConnection = NULL;

    /* Make sure the connection context can fit in the user buffer. */
    if((*pConnConfig).userBuffer.bufferLen < connectionUserBufferMinimumSize)
    {
        IotLogError("Buffer size is too small to initialize the connection context. User buffer size: %d, required minimum size; %d.", 
            (*pConnConfig).userBuffer.bufferLen, 
            connectionUserBufferMinimumSize);
        IOT_SET_AND_GOTO_CLEANUP(IOT_HTTPS_INSUFFICIENT_MEMORY);
    }
    /* Set the internal connection context to the start of the user buffer. */
    _httpsConnection = (_httpsConnection_t *)((*pConnConfig).userBuffer.pBuffer);

    /* Set to disconnected initially. */
    _httpsConnection->isConnected = false;
    
    /* Keep track of a non-persistent HTTP connection. If this flag is set to true, then we will call IotHttpsClient_Disconnect
       at the end of a synchronous or asychronous request. The end of a request is when the response has been fully received. */
    if( pConnConfig->flags & IOT_HTTPS_IS_NON_PERSISTENT_FLAG )
    {
        _httpsConnection->nonPersistent = true;
    }
    else
    {
        _httpsConnection->nonPersistent = false;
    }

    /* This timeout is used to wait for a response on the connection. */
    if( pConnConfig->timeout == 0 )
    {
        _httpsConnection->timeout = IOT_HTTPS_RESPONSE_WAIT_MS;
    }
    else
    {
        _httpsConnection->timeout = pConnConfig->timeout;
    }

    if( pConnConfig->pNetworkInterface == NULL)
    {
        IotLogError("pNetworkInterface in pConnConfig is NULL.");
        IOT_SET_AND_GOTO_CLEANUP(IOT_HTTPS_INVALID_PARAMETER);
    }
    /* pNetworkInterface contains the connect, disconnect, send, and receive over the network functions. */
    _httpsConnection->pNetworkInterface = ( IotNetworkInterface_t* )( pConnConfig->pNetworkInterface );
    
    /* If a pNetworkServer info configuration is passed in, then use it otherwise use the pHostName and port in the pConnConfig. */
    if(pConnConfig->pNetworkServerInfo != NULL)
    {
        memcpy(&networkServerInfo, pConnConfig->pNetworkServerInfo, sizeof(IotNetworkServerInfo_t));
    }
    else
    {
        /* IotNetworkServerInfo_t should take in the length of the host name instead of requiring a NULL terminator. */
        memcpy( pHostName, pConnConfig->pAddress, pConnConfig->addressLen );
        pHostName[pConnConfig->addressLen] = '\0';

        networkServerInfo.pHostName = pHostName; /* This requires a NULL terminated string. */
        networkServerInfo.port = pConnConfig->port;
    }

    /* If a pNetworkCredentialInfo is passed in, then use it otherwise, use the flags, Alpn Protocls, and certificates, 
       in the pConnConfig. */
    if( pConnConfig->pNetworkCredentialInfo != NULL)
    {
        memcpy(&networkCredentials, pConnConfig->pNetworkCredentialInfo, sizeof(IotNetworkCredentials_t));
    }
    else
    {
        /* If this is TLS connection, then set the network credentials. */
        if( ( pConnConfig->flags & IOT_HTTPS_IS_NON_TLS_FLAG ) == 0 )
        {
            if( pConnConfig->flags & IOT_HTTPS_DISABLE_SNI )
            {
                networkCredentials.disableSni = true;
            }
            else
            {
                networkCredentials.disableSni = false;
            }

            
            if( pConnConfig->pAlpnProtocols != NULL )
            {
                /* IotNetworkCredentials_t should take in a length for the alpn protocols string instead of requiring a 
                NULL terminator. */
                memcpy( pAlpnProtos, pConnConfig->pAlpnProtocols, pConnConfig->alpnProtocolsLen );
                pAlpnProtos[pConnConfig->alpnProtocolsLen] = '\0';
                networkCredentials.pAlpnProtos = pAlpnProtos; /* This requires a NULL termination. It is inconsistent with other members in the struct. */
            }
            else
            {
                networkCredentials.pAlpnProtos = NULL;
            }

            /* If any of these are NULL a network error will result depending on the connection. */
            networkCredentials.pRootCa = pConnConfig->pCaCert;
            networkCredentials.rootCaSize = pConnConfig->caCertLen;
            networkCredentials.pClientCert = pConnConfig->pClientCert;
            networkCredentials.clientCertSize = pConnConfig->clientCertLen;
            networkCredentials.pPrivateKey = pConnConfig->pPrivateKey;
            networkCredentials.privateKeySize = pConnConfig->privateKeyLen;
        }
    }

    /* If this is a TLS connection connect with networkCredentials. Otherwise pass NULL. */
    if( ( pConnConfig->flags & IOT_HTTPS_IS_NON_TLS_FLAG ) == 0 )
    {
        /* create() will connect to the server specified. */
        networkStatus = _httpsConnection->pNetworkInterface->create( &networkServerInfo,
            &networkCredentials,
            &( _httpsConnection->pNetworkConnection ) );
    }
    else
    {
        networkStatus = _httpsConnection->pNetworkInterface->create( &networkServerInfo,
            NULL,
            &( _httpsConnection->pNetworkConnection ) );
    }

    /* Check to see if the network connection succeeded. If we did not succeed then the connHandle
       will be NULL and we return an error. */
    if( networkStatus != IOT_NETWORK_SUCCESS )
    {
        IotLogError( "Failed to connect to the server at %.*s on port %d with error: %d",
            pConnConfig->addressLen,
            pConnConfig->pAddress,
            pConnConfig->port,
            networkStatus );
        IOT_SET_AND_GOTO_CLEANUP(IOT_HTTPS_CONNECTION_ERROR);
    }
    else
    {
        /* The connection succeeded so this a connected context. */
        _httpsConnection->isConnected = true;
    }

    /* The receive callback tells the task context handling the request/response that the network
       is ready to read from. */
    networkStatus = _httpsConnection->pNetworkInterface->setReceiveCallback( _httpsConnection->pNetworkConnection,
        _networkReceiveCallback,
        _httpsConnection );
    if( networkStatus != IOT_NETWORK_SUCCESS )
    {
        IotLogError( "Failed to connect to set the HTTPS receive callback. " );
        IOT_SET_AND_GOTO_CLEANUP(IOT_HTTPS_INTERNAL_ERROR);
    }

    /* Connection was successful, so create semaphores. */
    /* The overall lock on the connection is set to 1 to start so that the first
       thread to need to use it can use it. */
    if( IotSemaphore_Create( &( _httpsConnection->connSem ), 1, 1 ) == false )
    {
        IotLogError("Failed to create an internal connection semaphore.");
        IOT_SET_AND_GOTO_CLEANUP(IOT_HTTPS_INTERNAL_ERROR);
    }
    else
    {
        connSemCreated = true;
    }

    /* The reading start semaphore is initialized to 0 so that the network context can
       set it to 1 for a thread to read from the socket. */
    if( IotSemaphore_Create( &( _httpsConnection->rxStartSem ), 0, 1 ) == false )
    {
        IotLogError( "Failed to create an internal connection semaphore." );
        IOT_SET_AND_GOTO_CLEANUP(IOT_HTTPS_INTERNAL_ERROR);
    }
    else
    {
        rxStartSemCreated = true;
    }

    /* The reading finished semaphore is initialized to 0 that the application context
       can set it to 1 for the network context to return from the read ready callback. */
    if( IotSemaphore_Create( &( _httpsConnection->rxFinishSem ), 0, 1 ) == false )
    {
        IotLogError( "Failed to create an internal connection semaphore." );
        IOT_SET_AND_GOTO_CLEANUP(IOT_HTTPS_INTERNAL_ERROR);
    }
    else
    {
        rxFinishedSemCreated = true;
    }

    /* Return the new connection information. */
    *pConnHandle = _httpsConnection;

    IOT_FUNCTION_CLEANUP_BEGIN();

    /* If we failed anywhere in the connection process, then destroy the semaphores created. */
    if(status != IOT_HTTPS_OK)
    {
        /* If there was a connect was successful, disconnect from the network.  */
        if(( _httpsConnection != NULL) && (_httpsConnection->isConnected))
        {   
            if (_networkDisconnect(_httpsConnection) != IOT_HTTPS_OK)
            {
                IotLogError("Failed to disconnect from a failed connection context initialization.");
            }
        }

        if(connSemCreated)
        {
            IotSemaphore_Destroy( &(_httpsConnection->connSem) );
        }

        if(rxStartSemCreated)
        {
            IotSemaphore_Destroy( &(_httpsConnection->rxStartSem) );
        }

        if(rxFinishedSemCreated)
        {
            IotSemaphore_Destroy( &(_httpsConnection->rxFinishSem) );
        }
    }

    IOT_FUNCTION_CLEANUP_END();
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _networkDisconnect(_httpsConnection_t* _httpsConnection)
{
    IotNetworkError_t networkStatus = IOT_NETWORK_SUCCESS;
    IotHttpsReturnCode_t status = IOT_HTTPS_OK;

    networkStatus = _httpsConnection->pNetworkInterface->close( _httpsConnection->pNetworkConnection );
    if ( networkStatus != IOT_NETWORK_SUCCESS )
    {
        IotLogWarn("Failed to shutdown the socket with error code: %d", networkStatus );
        status = IOT_HTTPS_NETWORK_ERROR;
    }

    return status;
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_Disconnect(IotHttpsConnectionHandle_t connHandle)
{
    IotHttpsReturnCode_t status = IOT_HTTPS_OK;

    if ( connHandle == NULL )
    {
        IotLogError("NULL parameter passed into IotHttpsClient_Disconnect().");
        return IOT_HTTPS_INVALID_PARAMETER;
    }   

    /* Mark the network as disconnected whether the disconnect passed or not. */
    connHandle->isConnected = false;
    
    /* Disconnect from the network. */
    status = _networkDisconnect(connHandle);
    if( status != IOT_HTTPS_OK )
    {
        IotLogWarn("Failed to disconnect from the server with error code: %d", status);
    }

    /* Clean connection context resources. */
    IotSemaphore_Destroy( &(connHandle->connSem) );
    IotSemaphore_Destroy( &(connHandle->rxStartSem) );
    IotSemaphore_Destroy( &(connHandle->rxFinishSem) );
    return status;
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _addHeader(IotHttpsRequestHandle_t reqHandle, const char * pName, const char * pValue, uint32_t len )
{
    uint32_t additionalLength = strlen( pName ) + strlen( ": " ) + len + strlen( "\r\n" ) + 1; /* +1 for the the null terminating character written by snprintf(). */
    uint32_t possibleLastHeaderAdditionalLength = strlen(HTTPS_END_OF_HEADER_LINES_INDICATOR);
    int numWritten = 0;

    /* Check if the additional length needed for another header does not exceed the header buffer. */
    if( (additionalLength + possibleLastHeaderAdditionalLength + reqHandle->pHeadersCur) > (reqHandle->pHeadersEnd) )
    {
        IotLogError( "There is %d space left in the header buffer, but we want to add %d more of header.",
            reqHandle->pHeadersEnd - reqHandle->pHeadersCur,
            additionalLength + possibleLastHeaderAdditionalLength );
        return IOT_HTTPS_INSUFFICIENT_MEMORY;
    }

    /* numWritten does not include the NULL terminating character. */
    numWritten = snprintf(reqHandle->pHeadersCur,
                        additionalLength, 
                        "%s: %.*s\r\n",
                        pName,
                        len,
                        pValue);
    if( numWritten < 0 )
    {
        IotLogError("Failed to write \"%s: %.*s\r\n\" to the request user buffer. Error code: %d",
                        pName,
                        len,
                        pValue,
                        numWritten);
        return IOT_HTTPS_INTERNAL_ERROR;
    }
    reqHandle->pHeadersCur += numWritten;
    IotLogDebug("Wrote header: \"%s: %.*s\r\n\". Space left in request user buffer: %d", 
            pName,
            len,
            pValue,
            reqHandle->pHeadersEnd - reqHandle->pHeadersCur);

    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_InitializeRequest(IotHttpsRequestHandle_t * pReqHandle, IotHttpsRequestInfo_t *pReq)
{
    _httpsRequest_t * _httpsRequest = NULL;
    size_t additionalLength = 0;
    int numWritten = 0;
    IotHttpsReturnCode_t addHeaderStatus = IOT_HTTPS_OK;

    /* Check for NULL parameters in the public API. */
    if( ( pReqHandle == NULL) || ( pReq == NULL) )
    {
        IotLogError("NULL parameter passed into IotHttpsClient_InitializeRequest().");
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    /* Check of the user buffer is large enough for the request context + default headers. */
    if((*pReq).reqUserBuffer.bufferLen < requestUserBufferMinimumSize)
    {
        IotLogError("Buffer size is too small to initialize the request context. User buffer size: %d, required minimum size; %d.", 
            (*pReq).reqUserBuffer.pBuffer,
            requestUserBufferMinimumSize);
        return IOT_HTTPS_INSUFFICIENT_MEMORY;
    }
    /* Set the request contet to the start of the userbuffer. */
    _httpsRequest = ( _httpsRequest_t *)((*pReq).reqUserBuffer.pBuffer);

    /* Set the start of the headers to the end of the request context in the user buffer. */
    _httpsRequest->pHeaders = (uint8_t*)_httpsRequest + sizeof( _httpsRequest_t );
    _httpsRequest->pHeadersEnd = (uint8_t*)_httpsRequest + ( *pReq ).reqUserBuffer.bufferLen;
    _httpsRequest->pHeadersCur = _httpsRequest->pHeaders;

    /* Add the request line to the header buffer. The path in the req is not NULL terminated so we write each
       part individually. */
    additionalLength = strlen(httpsMethodStrings[(*pReq ).method]) + strlen(" ") + (*pReq).pathLen + strlen(" ") + strlen(HTTPS_PROTOCOL_VERSION) + strlen(HTTPS_END_OF_HEADER_LINES_INDICATOR);
    if( (additionalLength + _httpsRequest->pHeadersCur) > (_httpsRequest->pHeadersEnd ))
    {
        IotLogError("Request line does not fit into the request user buffer: \"%s %.*s HTTP/1.1\\r\\n\" . ",
            httpsMethodStrings[IOT_HTTPS_METHOD_GET],
            (*pReq).pathLen,
            (*pReq).pPath);
        IotLogError( "The length needed is %d and the space available is %d.", additionalLength, _httpsRequest->pHeadersEnd - _httpsRequest->pHeadersCur );
        return IOT_HTTPS_INSUFFICIENT_MEMORY;
    }

    numWritten = snprintf(_httpsRequest->pHeadersCur,
                              additionalLength + 1,
                              "%s %.*s HTTP/1.1\r\n",
                              httpsMethodStrings[(*pReq ).method],
                              (*pReq).pathLen,
                              (*pReq).pPath);
    if(numWritten < 0)
    {
        IotLogError("Failed to write Request line into the request user buffer: \"%s %.*s HTTP/1.1\\r\\n\" . Error code: %d",
            httpsMethodStrings[( *pReq ).method],
            (*pReq).pathLen,
            (*pReq).pPath,
            numWritten);
        return IOT_HTTPS_INTERNAL_ERROR;
    }
    _httpsRequest->pHeadersCur += numWritten;

    /* Add the required headers. */
    addHeaderStatus = _addHeader(_httpsRequest, "User-Agent", IOT_HTTPS_USER_AGENT, strlen( IOT_HTTPS_USER_AGENT ));
    if( addHeaderStatus != IOT_HTTPS_OK )
    {
        IotLogError("Failed to write header to the request user buffer: \"User-Agent: %s\\r\\n\" . Error code: %d", 
            IOT_HTTPS_USER_AGENT, 
            addHeaderStatus);
        return addHeaderStatus;
    }
    addHeaderStatus = _addHeader(_httpsRequest, "Host", ( *pReq ).pHost, ( *pReq ).hostLen);
    
    if( addHeaderStatus != IOT_HTTPS_OK )
    {
        IotLogError("Failed to write \"Host: %.*s\\r\\n\" to the request user buffer. Error code: %d", 
            ( *pReq ).hostLen, 
            ( *pReq ).pHost, 
            addHeaderStatus);
        return addHeaderStatus;
    }

    _httpsRequest->pBody = ( *pReq ).pSyncInfo->pReqData;
    _httpsRequest->bodyLength = ( *pReq ).pSyncInfo->reqDataLen;

    /* Save the connection info if the connection is to be made at the time of the request. */
    _httpsRequest->pConnInfo = ( *pReq ).pConnInfo;

    /* Initialize the corresponding response to this request. */
    if(( *pReq ).respUserBuffer.bufferLen < responseUserBufferMinimumSize)
    {
        IotLogError("Buffer size is too small to initialize the response context associated with this request. User buffer size: %d, required minimum size; %d.", 
            ( *pReq ).respUserBuffer.bufferLen, 
            responseUserBufferMinimumSize);
        return IOT_HTTPS_INSUFFICIENT_MEMORY;
    }
    _httpsRequest->pRespHandle = (_httpsResponse_t *)(( *pReq ).respUserBuffer.pBuffer);

    _httpsRequest->pRespHandle->pHeaders = (uint8_t*)(_httpsRequest->pRespHandle) + sizeof(_httpsResponse_t);
    _httpsRequest->pRespHandle->pHeadersEnd = (uint8_t*)(_httpsRequest->pRespHandle) + ( *pReq ).respUserBuffer.bufferLen;
    _httpsRequest->pRespHandle->pHeadersCur = _httpsRequest->pRespHandle->pHeaders;

    _httpsRequest->pRespHandle->pBody = ( *pReq ).pSyncInfo->pRespData;
    _httpsRequest->pRespHandle->pBodyCur = _httpsRequest->pRespHandle->pBody;
    _httpsRequest->pRespHandle->pBodyEnd = _httpsRequest->pRespHandle->pBody + ( *pReq ).pSyncInfo->respDataLen;

    /* Reinitialize the parser and set the fill buffer state to empty. */
    http_parser_init(&(_httpsRequest->pRespHandle->httpParser), HTTP_RESPONSE);

    _httpsRequest->pRespHandle->status = 0;
    _httpsRequest->pRespHandle->method = ( *pReq ).method;
    _httpsRequest->pRespHandle->contentLength = 0;
    _httpsRequest->pRespHandle->parserState = PARSER_STATE_NONE;
    _httpsRequest->pRespHandle->bufferProcessingState = PROCESSING_STATE_NONE;
    _httpsRequest->pRespHandle->pReadHeaderField = NULL;
    _httpsRequest->pRespHandle->pReadHeaderValue = NULL;
    _httpsRequest->pRespHandle->readHeaderValueLength = 0;
    _httpsRequest->pRespHandle->foundHeaderField = 0;
    _httpsRequest->pRespHandle->pConnHandle = NULL;
    _httpsRequest->pRespHandle->pReqHandle = _httpsRequest;

    *pReqHandle = _httpsRequest;
    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_AddHeader( IotHttpsRequestHandle_t reqHandle, char * pName, char * pValue, uint32_t len )
{
    /* Check for NULL pointer paramters. */
    if( (pName == NULL) || (pValue == NULL) || (reqHandle == NULL) )
    {
        IotLogError("Null parameter passed into IotHttpsClient_AddHeader().");
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    /* Check for auto-generated header "Content-Length". This header is created and send automatically when right before
       request body is sent on the network. */
    if( strncmp(pName, HTTPS_CONTENT_LENGTH_HEADER, strlen(HTTPS_CONTENT_LENGTH_HEADER)) == 0)
    {
        IotLogError("Attempting to add auto-generated header %s. This is not allowed.", HTTPS_CONTENT_LENGTH_HEADER);
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    /* Check for auto-generated header "Content-Length". This header is created and send automatically when right before
       request body is sent on the network. */
    if( strncmp(pName, HTTPS_CONNECTION_HEADER, strlen(HTTPS_CONNECTION_HEADER)) == 0)
    {
        IotLogError("Attempting to add auto-generated header %s. This is not allowed.", HTTPS_CONNECTION_HEADER);
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    /* Check for auto-generated header "Host". This header is created and placed into the header buffer space
       in IotHttpClient_InitializeRequest(). */
    if( strncmp(pName, HTTPS_HOST_HEADER, strlen(HTTPS_HOST_HEADER)) == 0)
    {
        IotLogError("Attempting to add auto-generated header %s. This is not allowed.", HTTPS_HOST_HEADER);
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    /* Check for auto-generated header "User-Agent". This header is created and placed into the header buffer space
       in IotHttpClient_InitializeRequest(). */
    if(strncmp(pName, HTTPS_USER_AGENT_HEADER, strlen(HTTPS_USER_AGENT_HEADER)) == 0)
    {
        IotLogError("Attempting to add auto-generated header %s. This is not allowed.", HTTPS_USER_AGENT_HEADER);
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    return _addHeader(reqHandle, pName, pValue, len );
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _networkSend(_httpsConnection_t* _httpsConnection, uint8_t * pBuf, size_t len)
{
    int32_t numBytesSent = 0;
    int32_t numBytesSentTotal = 0;
    int32_t sendLength = len;

    /* Send the headers first because the HTTPS body is in a separate pointer these are not contiguous. */
    while( numBytesSentTotal < sendLength )
    {
        numBytesSent = _httpsConnection->pNetworkInterface->send( _httpsConnection->pNetworkConnection, 
            &( pBuf[numBytesSentTotal] ), 
            sendLength - numBytesSentTotal );

        if( numBytesSent <= 0 )
        {
            IotLogError("Error in sending the HTTPS headers. Error code: %d", numBytesSent);
            break;
        }

        numBytesSentTotal += numBytesSent;
    }

    if( numBytesSentTotal != sendLength )
    {
        IotLogError("Error sending data on the network. We sent %d but there is %d left to send.", numBytesSentTotal, sendLength);
        return IOT_HTTPS_NETWORK_ERROR;
    }

    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _networkRecv( _httpsConnection_t* _httpsConnection, uint8_t * pBuf, size_t bufLen)
{
    int32_t numBytesRecv = 0;
    int32_t numBytesRecvTotal = 0;
    int32_t lengthToReceive = bufLen;
    IotHttpsReturnCode_t status = IOT_HTTPS_OK;

    do {
        numBytesRecv = _httpsConnection->pNetworkInterface->receive( _httpsConnection->pNetworkConnection,
            &( pBuf[ numBytesRecvTotal ] ),
            lengthToReceive - numBytesRecvTotal );

        if( numBytesRecv > 0 )
        {
            numBytesRecvTotal += numBytesRecv;
        }
        if( numBytesRecv == 0)
        {
            IotLogError("Timedout waiting for the HTTPS response message.");
            status = IOT_HTTPS_TIMEOUT_ERROR;
            break;
        }
        if( numBytesRecv < 0)
        {
            IotLogError("Error in receiving the HTTPS response message. Socket Error code %d", numBytesRecv);
            status = IOT_HTTPS_NETWORK_ERROR;
            break;
        }
    } while((numBytesRecv > 0)  && ( lengthToReceive - numBytesRecvTotal > 0));

    return status;   
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _sendHttpsHeaders( _httpsConnection_t* _httpsConnection, uint8_t* pHeadersBuf, uint32_t headersLength, bool isNonPersistent, uint32_t contentLength)
{
    const char* connectionHeader = NULL;
    IotHttpsReturnCode_t status = _networkSend( _httpsConnection, pHeadersBuf, headersLength);

    if( status != IOT_HTTPS_OK )
    {
        IotLogError("Error sending the HTTPS headers in the request user buffer. Error code: %d", status);
        return status;
    }

    char finalHeadersKeepAlive[HTTPS_MAX_CONTENT_LENGTH_LINE_LENGTH + HTTPS_CONNECTION_KEEP_ALIVE_HEADER_LINE_LENGTH + 1] = { 0 };
    
    if (isNonPersistent)
    {
        connectionHeader = HTTPS_CONNECTION_CLOSE_HEADER_LINE;
    }
    else
    {
        connectionHeader = HTTPS_CONNECTION_KEEP_ALIVE_HEADER_LINE;
    }

    if (contentLength > 0)
    {
        snprintf(finalHeadersKeepAlive, 
            sizeof(finalHeadersKeepAlive), 
            "%sContent-Length: %d\r\n\r\n",
            connectionHeader,
            contentLength);
    }
    else 
    {
        snprintf(finalHeadersKeepAlive, 
            sizeof(finalHeadersKeepAlive), 
            "%s\r\n",
            connectionHeader);
    }
    status = _networkSend( _httpsConnection, finalHeadersKeepAlive, strlen(finalHeadersKeepAlive));
    if( status != IOT_HTTPS_OK )
    {
        IotLogError("Error sending final HTTPS Headers \r\n%s. Error code: %d", finalHeadersKeepAlive, status);
        return status;
    }

    return IOT_HTTPS_OK;

}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _sendHttpsBody( _httpsConnection_t* _httpsConnection, uint8_t* pBodyBuf, uint32_t bodyLength)
{
    IotHttpsReturnCode_t status = _networkSend( _httpsConnection, pBodyBuf, bodyLength);
    if( status != IOT_HTTPS_OK )
    {
        IotLogError("Error sending final HTTPS body at location 0x%x. Error code: %d", pBodyBuf, status);
    }
    return status;
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _parseHttpsMessage(http_parser* pHttpParser, char* pBuf, size_t len)
{
    size_t parsedBytes = http_parser_execute(pHttpParser, &_httpParserSettings, pBuf, len );
    const char * pHttpParserErrorDescription = NULL;

    /* If the parser fails with HPE_CLOSED_CONNECTION or HPE_INVALID_CONSTANT that simply means there
       was data beyond the end of the message. We do not fail in this case because we often give the whole
       header buffer or body buffer to the parser even if it is only partly filled with data. 
       HPE_CB_message_begin <= error <= HPE_CB_chunk_complete covers all errors for returning a non-zer value 
       from the parser callbacks. I return a nonzero from the callback when I want to stop the parser early like
       for a HEAD request or when we find part of the response body in the header buffer. */
    if(pHttpParser->http_errno != 0 &&
        ( HTTP_PARSER_ERRNO( pHttpParser ) != HPE_CLOSED_CONNECTION ) &&
        ( HTTP_PARSER_ERRNO( pHttpParser ) != HPE_INVALID_CONSTANT ) &&
        ( ( HTTP_PARSER_ERRNO( pHttpParser ) >= HPE_CB_message_begin) &&
          ( HTTP_PARSER_ERRNO( pHttpParser ) <= HPE_CB_chunk_complete) ))
    {
        pHttpParserErrorDescription = http_errno_description( HTTP_PARSER_ERRNO( pHttpParser ) );
        IotLogError("http_parser failed on the http response with error: %s", pHttpParserErrorDescription);
        return IOT_HTTPS_PARSING_ERROR;
    }

    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _receiveHttpsMessage( _httpsConnection_t* _httpsConnection, 
    http_parser* pParser,
    IotHttpsResponseParserState_t *pCurrentParserState,
    IotHttpsResponseParserState_t finalParserState, 
    uint8_t** pBuf,
    uint8_t** pBufCur,
    uint8_t** pBufEnd,
    IotHttpsReturnCode_t *pNetworkStatus)
{    
    IotHttpsReturnCode_t status = IOT_HTTPS_OK;

    while( (*pCurrentParserState < finalParserState) && (*pBufEnd - *pBufCur > 0) ) 
    {
        *pNetworkStatus = _networkRecv( _httpsConnection,
            *pBufCur, 
            *pBufEnd - *pBufCur);

        status = _parseHttpsMessage(pParser, *pBufCur, *pBufEnd - *pBufCur);
        if(status != IOT_HTTPS_OK)
        {
            IotLogError("Failed to parse the message buffer with error: %d", pParser->http_errno);
            return status;
        }

        /* The _httResponse->pHeadersCur pointer is updated in the http_parser callbacks. */
        IotLogDebug( "There is %d of space left in the buffer.", *pBufEnd - *pBufCur );

        /* We cannot anticipate the unique network error received when the server closes the connection. 
           We simply exit the loop if there nothing else to receive. We do not return the network status because 
           the error may just be a server closed the connection, but there may still have been data in the buffer
           that we parsed. */
        if( *pNetworkStatus != IOT_HTTPS_OK )
        {
            IotLogError( "Error receiving the HTTPS response headers. Error code: %d", status );
            break;
        }

    }

    /* If we did not reach the end of the headers with the current header buffer size then log that info. */
    if( *pCurrentParserState < finalParserState)
    {
        IotLogDebug("There are still more data on the network. It could not fit into buffer at \
            location 0x%x with length %d.",
            *pBuf,
            *pBufEnd - *pBuf);
    }

    return status;
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _receiveHttpsHeaders( _httpsConnection_t* _httpsConnection, _httpsResponse_t* _httpsResponse, IotHttpsReturnCode_t *pNetworkStatus)
{
    IotHttpsReturnCode_t status = IOT_HTTPS_OK;

    /* Initialize the HTTPS parser for a new execution. */
    http_parser_init(&(_httpsResponse->httpParser), HTTP_RESPONSE);
    _httpsResponse->httpParser.data = (void *)(_httpsResponse);
    _httpsResponse->parserState = PARSER_STATE_NONE;
    _httpsResponse->bufferProcessingState = PROCESSING_STATE_FILLING_HEADER_BUFFER;

    status = _receiveHttpsMessage(_httpsConnection,
        &(_httpsResponse->httpParser),
        &(_httpsResponse->parserState),
        PARSER_STATE_HEADERS_COMPLETE,
        &(_httpsResponse->pHeaders),
        &(_httpsResponse->pHeadersCur),
        &(_httpsResponse->pHeadersEnd),
        pNetworkStatus);
    if( status != IOT_HTTPS_OK)
    {
        IotLogError("Error receiving the HTTP headers. Error code %d", status);
    }

    /* Whether the parser complete the headers or not we want to see if we got the Content-Length. */
    _httpsResponse->contentLength = _httpsResponse->httpParser.content_length;

    return status;
}

/*-----------------------------------------------------------*/

/* _receiveHttpsHeaders() must be called first before this function is called. */
static IotHttpsReturnCode_t _receiveHttpsBody( _httpsConnection_t* _httpsConnection, _httpsResponse_t* _httpsResponse, IotHttpsReturnCode_t *pNetworkStatus)
{
    IotHttpsReturnCode_t status = IOT_HTTPS_OK;
    _httpsResponse->bufferProcessingState = PROCESSING_STATE_FILLING_BODY_BUFFER;

    status = _receiveHttpsMessage(_httpsConnection,
        &(_httpsResponse->httpParser),
        &(_httpsResponse->parserState),
        PARSER_STATE_BODY_COMPLETE,
        &(_httpsResponse->pBody),
        &(_httpsResponse->pBodyCur),
        &(_httpsResponse->pBodyEnd),
        pNetworkStatus);
    if( status != IOT_HTTPS_OK)
    {
        IotLogError("Error receiving the HTTP body. Error code %d", status);
    }

    IotLogDebug("The message Content-Length is %d (Will be > 0 for a Content-Length header existing). The remaining content length on the network is %d.",
        _httpsResponse->contentLength,
        _httpsResponse->httpParser.content_length);

    return status;
}

/*-----------------------------------------------------------*/

static IotHttpsReturnCode_t _flushHttpsNetworkData( _httpsConnection_t* _httpsConnection, _httpsResponse_t* _httpsResponse )
{
    static uint8_t flushBuffer[IOT_HTTPS_MAX_FLUSH_BUFFER_SIZE] = { 0 };
    const char * pHttpParserErrorDescription = NULL;
    IotHttpsReturnCode_t parserStatus = IOT_HTTPS_OK;
    IotHttpsReturnCode_t networkStatus = IOT_HTTPS_OK;

    /* Even if there is not body, the parser state will become body complete after the headers finish. */
    while( _httpsResponse->parserState < PARSER_STATE_BODY_COMPLETE )
    {
        IotLogDebug( "Now clearing the rest of the response data on the socket. " );
        networkStatus = _networkRecv( _httpsConnection, flushBuffer, IOT_HTTPS_MAX_FLUSH_BUFFER_SIZE );

        /* Run this through the parser so that we can get the end of the HTTP message, instead of simply timing out the socket to stop.
           If we relied on the socket timeout to stop reading the network socket, then the server may close the connection. */
        parserStatus = _parseHttpsMessage(&(_httpsResponse->httpParser), flushBuffer, IOT_HTTPS_MAX_FLUSH_BUFFER_SIZE );
        if(parserStatus != IOT_HTTPS_OK)
        {
            pHttpParserErrorDescription = http_errno_description( HTTP_PARSER_ERRNO( &_httpsResponse->httpParser ) );
            IotLogError("Failed to parse the response body buffer with error: %d", _httpsResponse->httpParser.http_errno);
            return parserStatus;
        }

        /* If there is a network error then we want to stop clearing out the buffer. */
        if( networkStatus != IOT_HTTPS_OK )
        {
            IotLogError( "Error receiving the HTTPS response headers. Error code: %d", networkStatus );
            break;
        }
    }

    /* All network errors except timeout are returned. */
    if( networkStatus != IOT_HTTPS_TIMEOUT_ERROR)
    {
        return networkStatus;
    }

    /* If there is a timeout error just return the parser status. */
    return parserStatus;
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_SendSync(IotHttpsConnectionHandle_t *pConnHandle, IotHttpsRequestHandle_t reqHandle, IotHttpsResponseHandle_t * pRespHandle)
{

    IOT_FUNCTION_ENTRY(IotHttpsReturnCode_t, IOT_HTTPS_OK);

    _httpsResponse_t * _httpsResponse = NULL;
    _httpsConnection_t * _httpsConnection = NULL;
    IotHttpsReturnCode_t flushStatus = IOT_HTTPS_OK;

    /* Check for NULL parameters in a public API. */
    if(( pConnHandle == NULL) || (reqHandle == NULL) || (pRespHandle == NULL))
    {
        IotLogError("NULL parameter passed into IotHttpsClient_SendSync()");
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    /* Set the internal response context to use and the response handle to return. */
    _httpsResponse = reqHandle->pRespHandle;
    *pRespHandle = _httpsResponse;

    /* If the pConnHandle points to a NULL handle or the pConnHandle is false, then make the connection now. */
    if( (* pConnHandle == NULL) || ((* pConnHandle)->isConnected == false ))
    {
        /* In order to make the connection now the pConnInfo member of IotHttpsRequestHandle_t must not be NULL. */
        if(reqHandle->pConnInfo == NULL)
        {
            IotLogError("IotHttpsRequestInfo_t should have been confogired with pConnInfo not NULL in IotHttpsClient_InitializeRequest() in order to connect implicitly.");
            IOT_GOTO_CLEANUP();
        }
        status = _connectHttpsServer(&(reqHandle->pConnHandle), reqHandle->pConnInfo);
        if(status != IOT_HTTPS_OK)
        {
            IotLogError("An error occurred in connecting with th server with error code: %d", status);
            IOT_GOTO_CLEANUP();
        }
    }
    /* Else pConnHandle is not null and it is connected (DeMorgans law). */
    else
    {
        reqHandle->pConnHandle = *pConnHandle;
    }

    /* Set the connection handles to return. */
    *pConnHandle = reqHandle->pConnHandle;
    *pRespHandle = reqHandle->pRespHandle;
    ( *pRespHandle )->pConnHandle = *pConnHandle;
    ( *pRespHandle )->pReqHandle = reqHandle;

    /* Set the internal connection context since we are connected now. */
    _httpsConnection = *pConnHandle;

    /* Lock the entire connection for sending the request and receiving the response. */
    if( IotSemaphore_TimedWait( &(_httpsConnection->connSem), IOT_HTTPS_MAX_CONN_USAGE_WAIT_MS ) == true )
    {
        /* Send the headers first. Because the HTTPS body is in a separate pointer. */
        status = _sendHttpsHeaders( _httpsConnection,
            reqHandle->pHeaders,
            reqHandle->pHeadersCur - reqHandle->pHeaders,
            _httpsConnection->nonPersistent,
            reqHandle->bodyLength);

        if( status != IOT_HTTPS_OK )
        {
            IotLogError("Error sending the HTTPS headers in the request user buffer. Error code: %d", status);
            IOT_GOTO_CLEANUP();
        }

        /* Send the body now if it exists. */
        if(reqHandle->pBody != NULL)
        {
            status = _sendHttpsBody( _httpsConnection, reqHandle->pBody, reqHandle->bodyLength);
            if( status != IOT_HTTPS_OK )
            {
                IotLogError("Error sending final HTTPS body. Return code: %d", status);
                IOT_GOTO_CLEANUP(); 
            }
        }

        /* Wait for the network to have data to read.*/
        if( IotSemaphore_TimedWait( &( _httpsConnection->rxStartSem ), _httpsConnection->timeout ) == false )
        {
            IotLogError( "Timed out waiting for a response from the network." );
            status = IOT_HTTPS_TIMEOUT_ERROR;
            IOT_GOTO_CLEANUP();
        }

        IotHttpsReturnCode_t networkStatus;
        status = _receiveHttpsHeaders( _httpsConnection, _httpsResponse, &networkStatus);
        if(status != IOT_HTTPS_OK)
        {
            IotLogError("Error in receiving the HTTPS headers into user header buffer at 0x%x. Error code: %d.", _httpsResponse->pHeaders, status);
            IOT_GOTO_CLEANUP(); 
        }

        if( _httpsResponse->parserState < PARSER_STATE_HEADERS_COMPLETE )
        {
            IotLogDebug( "Headers received on the network did not all fit into the configured header buffer. \
                The length of the headers buffer is: %d",
                _httpsResponse->pHeadersEnd - _httpsResponse->pHeaders );
            /* It is not error if the headers did not all fit into the buffer. */
            //status = IOT_HTTPS_MESSAGE_TOO_LARGE;

            /* If we are not expecting a body, then we want to flush the socket. */
            if( _httpsResponse->pBody == NULL )
            {
                IOT_GOTO_CLEANUP();
            }
        }

        /* The header buffer is now filled or the end of the headers has been reached already. If part of the response
           body was read from the network into the header buffer, then it was already copied to the body buffer in the 
           _httpParserOnBodyCallback(). */
        if(_httpsResponse->pBody != NULL)
        {
            /* If there is room left in the body buffer, then try to receive more. */
            if( (_httpsResponse->pBodyEnd - _httpsResponse->pBodyCur) > 0 )
            {
                IotHttpsReturnCode_t networkStatus;
                status = _receiveHttpsBody( _httpsConnection,
                    _httpsResponse,
                    &networkStatus );
                if( status != IOT_HTTPS_OK )
                {
                    IotLogError( "Error receiving the HTTPS response body. Error code: %d", status );
                    IOT_GOTO_CLEANUP();
                }
            }

            /* If we don't reach the end of the HTTPS body in the parser, then we only received part of the body.
               The rest of body will be on the socket. */
            if( _httpsResponse->parserState < PARSER_STATE_BODY_COMPLETE )
            {
                IotLogError( "HTTPS response body does not fit into application provided response buffer at location \
                0x%x with length: %d",
                    _httpsResponse->pBody,
                    _httpsResponse->pBodyEnd - _httpsResponse->pBody );
                status = IOT_HTTPS_MESSAGE_TOO_LARGE;
                IOT_GOTO_CLEANUP();
            }
        }

        IOT_FUNCTION_CLEANUP_BEGIN();

        /* Flush the socket of the rest of the data if there is data left from this response. We need to do this
           so that for the next request on this connection, there is not left over response from this request in
           the next response buffer.

           If a continuous stream of data is coming in from the connection, with an unknown end, we may not be able to
           flush the network data. It may sit here forever. A continuous stream should be ingested with the async workflow. 
           
           All network errors are ignore here because network read will have read the data from network buffer despite
           errors. */
        if( _httpsConnection != NULL )
        {
            flushStatus = _flushHttpsNetworkData( _httpsConnection, _httpsResponse );
        }
        
        if( flushStatus == IOT_HTTPS_PARSING_ERROR)
        {
            IotLogWarn("There an error parsing the network flush data. The network buffer might not be fully flushed.");
        }

        /* If this is not a persistent connection, the server would have closed it after
           sending a response, but we disconnect anyways. */
        if( _httpsConnection->nonPersistent )
        {
            status = IotHttpsClient_Disconnect( _httpsConnection );
            if( status != IOT_HTTPS_OK )
            {
                IotLogError( "Failed to disconnected from the server with error: %d", status );
                /* The disconnect status is not returned as the server may already be disconnected. */
            }
        }

        /* Set the buffer state to finished for both success or error. This is so that the application can still
           read headers copied even if there was an error. PROCESSING_STATE_FINISHED state is checked in the https parsing 
           callacks to know if we are currently parsing a new response or we are simply parsing to read header values.*/
        _httpsResponse->bufferProcessingState = PROCESSING_STATE_FINISHED;

        /* Let the network thread know that reading from the socket is finished. Whatever context called
           the network receive callback will return from that callback now. */
        IotSemaphore_Post( &( _httpsConnection->rxFinishSem ) );
        /* Let other threads waiting know the connection is ready to be used. */
        IotSemaphore_Post( &(_httpsConnection->connSem) );
        IOT_FUNCTION_CLEANUP_END();
    }
    else
    {
        IotLogError("Timed out in sync send waiting on the connection handle to be free. The current timeout is %d.", IOT_HTTPS_MAX_CONN_USAGE_WAIT_MS);
        return IOT_HTTPS_BUSY;
    }
    
    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_ReadResponseStatus(IotHttpsResponseHandle_t respHandle, uint16_t *status)
{
    if((respHandle == NULL) || (status == NULL))
    {
        IotLogError("NULL parameter passed into IotHttpsClient_ReadResponseStatus().");
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    if( respHandle->status == 0)
    {
        IotLogError("The HTTP response status was not found in the HTTP response header buffer.");
        return IOT_HTTPS_NOT_FOUND;
    }
    *status = respHandle->status;
    
    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_ReadHeader(IotHttpsResponseHandle_t respHandle, char *pName, char *pValue, uint32_t len)
{
    IotHttpsResponseBufferState_t savedState = respHandle->bufferProcessingState;

    if((respHandle == NULL) || (pName == NULL) || (pValue == NULL))
    {
        IotLogError("NULL parameter passed into IotHttpsClient_ReadResponseStatus().");
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    respHandle->pReadHeaderField = pName;
    respHandle->foundHeaderField = false;
    respHandle->bufferProcessingState = PROCESSING_STATE_SEARCHING_HEADER_BUFFER;

    http_parser_init( &( respHandle->httpParser ), HTTP_RESPONSE );
    http_parser_execute(&(respHandle->httpParser), &_httpParserSettings, respHandle->pHeaders, respHandle->pHeadersCur - respHandle->pHeaders);
    respHandle->bufferProcessingState = savedState;
    if(respHandle->foundHeaderField)
    {
        if(respHandle->readHeaderValueLength > len)
        {
            IotLogError("IotHttpsClient_ReadHeader(): The length of the pValue buffer specified is less than the actual length of the pValue. ");
            return IOT_HTTPS_INSUFFICIENT_MEMORY;
        }
        strncpy(pValue, respHandle->pReadHeaderValue, respHandle->readHeaderValueLength);
    }
    else
    {
        IotLogError("IotHttpsClient_ReadHeader(): The header field %s was not found.", pName);
        return IOT_HTTPS_NOT_FOUND;
    }
    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/

IotHttpsReturnCode_t IotHttpsClient_ReadContentLength(IotHttpsResponseHandle_t respHandle, uint32_t *contentLength)
{   
    /* Check for NULL parameters in this public API. */
    if((respHandle == NULL) || (contentLength == NULL))
    {
        IotLogError("NULL parameter passed into IotHttpsClient_ReadContentLength().");
        return IOT_HTTPS_INVALID_PARAMETER;
    }

    /* If there is no content-length header or if we were not able to store it in the header buffer this will be
       invalid. */
    if(respHandle->contentLength == 0)
    {
        IotLogError("The content length not found in the HTTP response header buffer.");
        return IOT_HTTPS_NOT_FOUND;
    }

    *contentLength = respHandle->contentLength;

    return IOT_HTTPS_OK;
}

/*-----------------------------------------------------------*/
