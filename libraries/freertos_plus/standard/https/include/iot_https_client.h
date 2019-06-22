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
 * @brief User-facing functions of the Amazon FreeRTOS HTTPS Client library. 
 */

#ifndef IOT_HTTPS_CLIENT_H_
#define IOT_HTTPS_CLIENT_H_

/* The config header is always included first. */
#include "iot_config.h"

/* HTTP types include. */
#include "types/iot_https_types.h"

/**
 * @functionspage{https_client,HTTPS Client Library}
 * - @functionname{https_client_function_init}
 * - @functionname{https_client_function_deinit}
 * - @functionname{https_client_function_connect}
 * - @functionname{https_client_function_disconnect}
 * - @functionname{https_client_function_initializerequest}
 * - @functionname{https_client_function_addheader}
 * - @functionname{https_client_function_writerequestbody}
 * - @functionname{https_client_function_sendsync}
 * - @functionname{https_client_function_sendasync}
 * - @functionname{https_client_function_cancelrequestasync}
 * - @functionname{https_client_function_readresponsestatus}
 * - @functionname{https_client_function_readcontentlength}
 * - @functionname{https_client_function_readheader}
 * - @functionname{https_client_function_readresponsebody}
 */

/**
 * @functionpage{IotHttpsClient_Init,https_client,init}
 * @functionpage{IotHttpsClient_Deinit,https_client,deinit}
 * @functionpage{IotHttpsClient_Connect,https_client,connect}
 * @functionpage{IotHttpsClient_Disconnect,https_client,disconnect}
 * @functionpage{IotHttpsClient_InitializeRequest,https_client,initializerequest}
 * @functionpage{IotHttpsClient_AddHeader,https_client,addheader}
 * @functionpage{IotHttpsClient_WriteRequestBody,https_client,writerequestbody}
 * @functionpage{IotHttpsClient_SendSync,https_client,sendsync}
 * @functionpage{IotHttpsClient_SendAsync,https_client,sendasync}
 * @functionpage{IotHttpsClient_CancelRequestAsync,https_client,cancelrequestasync}
 * @functionpage{IotHttpsClient_ReadResponseStatus,https_client,readresponsestatus}
 * @functionpage{IotHttpsClient_ReadContentLength,https_client,readcontentlength}
 * @functionpage{IotHttpsClient_ReadHeader,https_client,readheader}
 * @functionpage{IotHttpsClient_ReadResponseBody,https_client,readresponsebody}
 */

/**
 * @brief One-time initialization of the IoT HTTPS Client library. 
 * 
 * <b>This must be called once before calling any API.</b> 
 * 
 * @warning No thread safety guarantees are provided for this function.
 * 
 * @return - #IOT_HTTPS_OK if the HTTPS library as successfully initialized. 
 *         - Failure code otherwise. Please see #IotHttpsReturnCode_t for the failure codes.
 * 
 * @see @ref https_client_function_deinit
 */
/* @[declare_https_client_init] */
IotHttpsReturnCode_t IotHttpsClient_Init( void );
/* @[declare_https_client_init] */

/**
 * @brief One time deinitialization of the IoT HTTPS Client library. 
 * 
 * This function frees resources taken in in @ref https_client_function_init. It should be called after
 * all HTTPS Connections represented by #IotHttpsConnectionHandle_t returned @ref https_client_function_sendasync, and
 * @ref https_client_function_sendsync have been closed. After this function returns @ref https_client_function_init 
 * must be called again to use this library. 
 * 
 * @warning No thread safety guarantees are provided for this function.
 */
/* @[declare_https_client_deinit] */
void IotHttpsClient_Deinit( void );
/* @[declare_https_client_deinit] */

/**
 * @brief Disconnect from the HTTPS server given the connection handle connHandle.
 * 
 * This routine blocks until the disconnect is complete. 
 * If the connection handle is not valid, the behavior is undefined.
 * If the connection handle is already disconnected then this routine will return IOT_HTTPS_OK.
 * 
 * This API MUST always be called on a valid #IotHttpsConnectionHandle_t whether is was created explicitly with 
 * @ref https_client_function_connect or implicitly with @ref https_client_function_sendsync 
 * @ref https_client_function_sendasync.
 * 
 * @param[in] connHandle - Valid handle representing an open connection.
 * 
 * @return #IOT_HTTPS_OK if the disconnect was successful
 *         #IOT_HTTPS_NETWORK_ERROR for network disconnection errors.
 *         See #IotHttpsReturnCode_t for other failure codes.
 */
/* @[declare_https_client_disconnect] */
IotHttpsReturnCode_t IotHttpsClient_Disconnect(IotHttpsConnectionHandle_t connHandle);
/* @[declare_https_client_disconnect] */

/**
 * @brief Initializes the request by adding a formatted Request Line to the HTTPS request header buffer. 
 * 
 * The request line will be added to the headers space in #IotHttpsRequestInfo_t.reqUserBuffer. The header space follows
 * the request context in the user buffer. See @ref requestUserBufferMinimumSize for more information on sizing the 
 * #IotHttpsRequestInfo_t.reqUserBuffer so that this function does not fail.
 * 
 * The request line generated is of the following format:
 * 
 * "method path version\r\n"
 * 
 * Example:
 * 
 * "GET /path/to/item.file?possible_query HTTP/1.1\r\n"
 * 
 * The initial required headers are also added to the #IotHttpsRequestInfo_t.reqUserBuffer. These headers are User-Agent
 * and Host. The User-Agent value is configured in iot_config.h using IOT_HTTPS_USER_AGENT. The Host value is the DNS 
 * resolvable server address.
 * 
 * @param[out] pReqHandle - request handle representing the internal request context is returned.
 * @param[in] pReq - HTTPS request information.
 * 
 * @return #IOT_HTTPS_OK if the request line was successfully added to req.request_message.headers.
 *         #IOT_HTTPS_INSUFFICIENT_MEMORY if the request line generated exceeds req.request_message.headers_len or the 
 *          user buffer is too small.
 *         #IOT_HTTPS_INVALID_PARAMETER for NULL parameters.
 *         #IOT_HTTPS_INTERNAL_ERROR for library internal errors.
 *         Please see #IotHttpsReturnCode_t for other failure codes.
 */
/* @[declare_https_client_initializerequest] */
IotHttpsReturnCode_t IotHttpsClient_InitializeRequest(IotHttpsRequestHandle_t * pReqHandle, IotHttpsRequestInfo_t *pReq);
/* @[declare_https_client_initializerequest] */

/**
 * @brief Add a header to the current HTTPS request represented by reqHandle.
 * 
 * The header line is appended to the request header buffer space in #IotHttpsRequestInfo_t.reqUserBuffer. 
 * Please see #requestUserBufferMinimumSize for information about sizing the #IotHttpsRequestInfo_t.reqUserBuffer so
 * that this function does not fail.
 * 
 * Header lines are appended in the following format:
 * 
 * "header_field_name: header_value\r\n"
 * 
 * Example:
 * 
 * "Range: bytes=1024-2047\r\n"
 * 
 * The last header line requires a "\r\n" following to separate the last header line from the entity body. 
 * These 2 characters are accounted for in #requestUserBufferMinimumSize.
 * 
 * The remaining length, after the header is added, is printed to the system configured standard debug output when 
 * IOT_LOG_LEVEL_HTTPS is set to IOT_LOG_DEBUG in iot_config.h. 
 * 
 * For a synchronous request, if extra headers are desired to be added, this function must be invoked before 
 * @ref https_client_function_sendsync.
 * <b> Synchronous Example </b>
 * @code{c}
 * ...
 * char * date_in_iso8601[17];
 * GET_DATE_IN_ISO8601(date_in_iso8601);
 * IotHttpsClient_AddHeader(reqHandle, "x-amz-date", date_in_iso8601, strlen(date_in_iso8601));
 * ...
 * IotHttpsClient_SendSync(&connHandle, reqHandle, &respHandle);
 * ...
 * @endcode
 * 
 * The following header fields are automatically added to the request header buffer and should be NOT added again with 
 * this routine:
 * - Connection:
 * - User-agent:
 * - Host:
 * - Content-Length:
 *
 * @param[in] reqHandle - HTTPS request to write the header line to.
 * @param[in] pName - String header field name to write. This is NULL terminated.
 * @param[in] pValue - https header value buffer pointer. Do not include token name.
 * @param[in] len - length of header value to write.
 * 
 * @return #IOT_HTTPS_OK if the header line was successfully added to req.request_message.headers.
 *         #IOT_HTTPS_INSUFFICIENT_MEMORY if the header line cannot fit into the header buffer.
 *         #IOT_HTTPS_INVALID_PARAMETER for NULL parameters or if an attempt to add automatic headers is done.
 *         Please see #IotHttpsReturnCode_t for other failure codes.
 */
/* @[declare_https_client_addheader] */
IotHttpsReturnCode_t IotHttpsClient_AddHeader(IotHttpsRequestHandle_t reqHandle, char * pName, char * pValue, uint32_t len);
/* @[declare_https_client_addheader] */

/**
 * @brief Synchronous send of the the HTTPS request.
 * 
 * This function blocks waiting for the entirety of sending the request and receiving the response. 
 * 
 * #IotHttpsRequestInfo_t.pSyncInfo must have been configured to send a request synchronously. If the
 * request was configured with #IotHttpsRequestInfo.pAsyncInfo instead, invoking this function will have 
 * undefined behavior. 
 * 
 * If parameter pConnHandle is passed in pointing to a NULL #IotHttpsConnectionHandle_t, then a connection is to be made 
 * implicitly and a valid connection handle in pConnHandle will be returned. 
 * If parameter pConnHandle points to a non-NULL #IotHttpsConnectionHandle_t and it is in a disconnected state, then a 
 * connection is to be made implicitly and valid connection handle in pConnHandle will be returned. 
 * In each implicit connection case #IotHttpsRequestHandle_t reqHandle must have been returned from 
 * @ref https_client_function_initializerequest where the #IotHttpsRequestInfo_t request configuration contained a 
 * non-NULL #IotHttpsRequestInfo_t.pConnInfo. Please see #IotHttpsConnectionInfo_t for connection configurations. 
 * 
 * For am implicit connection:
 * This function opens a new HTTPS connection between the server specified in #IotHttpsConnectionInfo_t. The connection 
 * is established by default on top of TLS over TCP. If the application wants to connect over TCP only, then it must
 * add the @ref IOT_HTTPS_IS_NON_TLS_FLAG to #IotHttpsConnectionInfo_t.flags. This is done at the applications own risk.
 * 
 * If @ref IOT_HTTPS_IS_NON_PERSISTENT_FLAG is set in #IotHttpsConnectionInfo_t.flags, then the connection will 
 * disconnect, close, and clean all taken resources automatically after receiving the first response.
 * 
 * @ref https_client_function_disconnect must always be called when an implicit connection is made for a 
 * persistent connection or if the first response is never received to cause automatic disconnect in a 
 * non-persistent connection. 
 * 
 * See @ref connectionUserBufferMinimumSize for information about the user buffer configured in 
 * #IotHttpsConnectionInfo_t.userBuffer needed to create a valid connection handle.
 * 
 * To retrieve the response body applications must directly refer to the #pSyncInfo_t.pRespData configured in 
 * #IotHttpsRequestInfo_t.pSyncInfo_t. Otherwise this function will return an #IOT_HTTPS_INVALID_PARAMETER error code. 
 * This function is intended to read the response entity body from the network and the synchronous response process 
 * handles all of that in @ref https_client_function_sendsync.
 * 
 * If the response body does not fit in the configured #IotHttpsRequestInfo_t.pSyncInfo.pRespData, then 
 * #IotHttpsRequestInfo_t.pSyncInfo.pRespData will contain only the first #IotHttpsRequestInfo_t.pSyncInfo.respDataLen 
 * of the body requested and the rest of the body requested will be thrown away. This function will return with error 
 * #IOT_HTTPS_MESSAGE_TOO_LARGE if the response body does fit into #IotHttpsRequestInfo_t.pSyncInfo.pRespData. To avoid
 * this issue, the application needs to determine beforehand how large the file to download is. This can be done with a
 * HEAD request first, then extracting the "Content-Length" with @ref https_client_function_readcontentlength. This 
 * could also be done with  a GET request with the header "Range: bytes=0-0", then extracing the "Content-Range" with 
 * @ref https_client_function_readheader.
 * 
 * The response headers as received from the network will be stored in the header buffer space in
 * #IotHttpsRequestInfo_t.respUserBuffer. If the configured #IotHttpsRequestInfo_t.respUserBuffer is too small to fit 
 * the headers received, then headers that don't fit will be thrown away. Please see #responseUserBufferMinimumSize for 
 * information about sizing the #IotHttpsRequestInfo_t.respUserBuffer.
 * 
 * @param[in out] pConnHandle - Handle from an HTTPS connection. If points to NULL then an implicit connection will be made.
 * @param[in] reqHandle - Handle from a request created with IotHttpsClient_initialize_request.
 * @param[out] pRespHandle - HTTPS response handle resulting from a successful send and receive.
 * 
 * @return  #IOT_HTTPS_OK - if the request was sent and the response was received successfully.
 *          #IOT_HTTPS_MESSAGE_TOO_LARGE if the response cannot fit in the configured 
 *              struct IotHttpsRequestHandle.respUserBuffer.pBuffer and struct IotHttpsRequestHandle.pSyncInfo.pRespData.
 *          #IOT_HTTPS_CONNECTION_ERROR if the connection failed.
 *          #IOT_HTTPS_INVALID_PARAMETER if there are NULL parameters or the request is asynchronous.
 *          #IOT_HTTPS_NETWORK_ERROR if there was an error sending the data on the network.
 *          #IOT_HTTPS_PARSING_ERROR if there was an error parsing the HTTP response.
 *          Please see #IotHttpsReturnCode_t for other failure codes.
 */
/* @[declare_https_client_sendsync] */
IotHttpsReturnCode_t IotHttpsClient_SendSync(IotHttpsConnectionHandle_t* pConnHandle, IotHttpsRequestHandle_t reqHandle, IotHttpsResponseHandle_t * pRespHandle);
/* @[declare_https_client_sendsync] */

/**
 * @brief Retrieve the HTTPS response status.
 * 
 * The HTTP response status code is contained in the status line of the response header buffer configured in 
 * #IotHttpsRequestInfo_t.respUserBuffer. It is the first line of a standard HTTP response message. If the response 
 * status line could not fit into #IotHttpsRequestInfo_t.respUserBuffer, then this function will return an error code. 
 * Please see #responseUserBufferMinimumSize for information about sizing the #IotHttpsRequestInfo_t.respUserBuffer.
 * 
 * This routine is primarily to be used for a synchronous response. It can be used for an asynchronous response.
 * <b> Example Synchronous Code </b>
 * @code{c}
 *      ...
 *      IotHttpsClient_SendSync(&connHandle, reqHandle, &respHandle);
 *      uint16_t status = 0;
 *      IotHttpsClient_ReadResponseStatus(respHandle, &status);
 *      if (status != IOT_HTTPS_STATUS_OK)
 *      {
 *          // Handle error server response status.
 *      }
 *      ...
 * @endcode
 * 
 * param[in] respHandle - Unique handle representing the HTTPS response.
 * param[out] pStatus - Integer status returned from the server response.
 * 
 * @return #IOT_HTTPS_OK if the response status was successfully read into *status.
 *         #IOT_HTTPS_INVALID_PARAMETER for NULL parameters.
 *         #IOT_HTTPS_NOT_FOUND if the HTTP response status was not found in the header buffer.
 *         Error code otherwise, please see #IotHttpsReturnCode_t for for failure codes.
 */
/* @[declare_https_client_readresponsestatus] */
IotHttpsReturnCode_t IotHttpsClient_ReadResponseStatus(IotHttpsResponseHandle_t respHandle, uint16_t *pStatus);
/* @[declare_https_client_readresponsestatus] */

/**
 * @brief Retrieve the HTTPS response content length. 
 * 
 * If the "Content-Length" header is available in #IotHttpsRequestInfo_t.respUserBuffer, this routine extracts that 
 * value. The "Content-Length" header could not be available because the server sent a multi-part encoded response 
 * (For example, "Transfer-Encoding: chunked") or the "Content-Length" header was far down in the list of response 
 * headers and could not fit into the header buffer. Please see #responseUserBufferMinimumSize for information about 
 * sizing the #IotHttpsRequestInfo_t.respUserBuffer.
 * 
 * In a synchronous request process, the Content-Length is available after @ref https_client_function_sendsync has 
 * returned successfully.
 * <b> Example Synchronous Code </b>
 * @code{c}
 *      ...
 *      IotHttpsClient_SendSync(&connHandle, reqHandle, &respHandle);
 *      uint32_t contentLength = 0;
 *      IotHttpsClient_ReadContentLength(respHandle, &contentLength);
 *      printf("Content-Length: %d", contentLength);
 *      ...
 * @endcode
 * 
 * param[in] respHandle - Unique handle representing the HTTPS response.
 * param[out] pContentLength - Integer content length from the Content-Length header from the server.
 * 
 * @return #IOT_HTTPS_OK if the response body Content-Length was successfully read into contentLength.
 *         #IOT_HTTPS_NOT_FOUND if the Content-Length header was not found in the header buffer.
 *         #IOT_HTTPS_INVALID_PARAMETER if NULL parameters are passed in.
 *         Error code otherwise, please see #IotHttpsReturnCode_t for for failure codes.
 */
/* @[declare_https_client_readcontentlength] */
IotHttpsReturnCode_t IotHttpsClient_ReadContentLength( IotHttpsResponseHandle_t respHandle, uint32_t *pContentLength );
/* @[declare_https_client_readcontentlength] */

/**
 * @brief Retrieve the header of interest from the response represented by respHandle.
 * 
 * The response headers as received from the network will be stored in the header buffer space in
 * #IotHttpsRequestInfo_t.respUserBuffer. If the configured #IotHttpsRequestInfo_t.respUserBuffer is too small to fit 
 * the headers received, then headers that don't fit will be thrown away. Please see #responseUserBufferMinimumSize for 
 * information about sizing the #IotHttpsRequestInfo_t.respUserBuffer.
 * 
 * This routine parses the formatted HTTPS header lines in the header buffer for the header field name specified. If the
 * header is not available, then #IOT_HTTPS_NOT_FOUND is returned.
 * 
 * For a syncrhonous response, this routine is to be called after @ref https_client_function_sendsync has 
 * returned successfully.
 * <b> Example Synchronous Code </b>
 * @code{c}
 *      ...
 *      IotHttpsClient_SendSync(&connHandle, reqHandle, &respHandle);
 *      char valueBuf[10];
 *      IotHttpsClient_readHeader(respHandle, "Content-Length", valueBuf, sizeof(valueBuf));
 *      uint32_t length = strtoul(valueBuf, NULL, 10);
 *      ...
 * @endcode
 * 
 * @param[in] respHandle - Unique handle representing the HTTPS response.
 * @param[in] name - HTTPS Header field name we want the value of. This should be NULL terminated.
 * @param[out] value - Buffer to hold the HTTPS field's value. This will be returned NULL terminated.
 * @param[in] len - The length of the value buffer.
 * 
 * @return #IOT_HTTPS_OK if the header's corresponding value was read into *value. 
 *         #IOT_HTTPS_NOT_FOUND if the header value was not found.
 *         #IOT_HTTPS_INVALID_PARAMETER if the resHandle is not valid. There is no response saved or the handle does not exist.
 *         #IOT_HTTPS_INSUFFICIENT_MEMORY if the value is too large to fit into *value.
 *         Please see #IotHttpsReturnCode_t for other failure codes.
 */
/* @[declare_https_client_readheader] */
IotHttpsReturnCode_t IotHttpsClient_ReadHeader(IotHttpsResponseHandle_t respHandle, char *pName, char *pValue, uint32_t len);
/* @[declare_https_client_readheader] */

#endif /* IOT_HTTPS_CLIENT_ */
