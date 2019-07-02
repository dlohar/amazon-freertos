/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "FreeRTOS_Sockets.h"

/* FreeRTOS+TCP includes. */
#include "iot_https_client.h"
#include "iot_https_internal.h"

/* stubbing library functions */
#define HTTPS_MAX_CONTENT_LENGTH_LINE_LENGTH    ( 26 )

int snprintf( char * contentLengthHeaderStr, size_t len, const char * data,  const char * content, uint32_t contentLength) {
  int ret;
  //__CPROVER_array_copy(contentLengthHeaderStr, data);
  //ret = strlen(data);
  __CPROVER_assume(ret >= 0 && ret <= HTTPS_MAX_CONTENT_LENGTH_LINE_LENGTH);
  return ret;
}

IotHttpsReturnCode_t _networkSend(_httpsConnection_t* _httpsConnection, uint8_t * pBuf, size_t len) {
  IotHttpsReturnCode_t code;
  return code;
}

IotHttpsReturnCode_t _networkRecv( _httpsConnection_t* _httpsConnection, uint8_t * pBuf, size_t bufLen) {
  IotHttpsReturnCode_t code;
  return code;
}

IotHttpsReturnCode_t _parseHttpsMessage(http_parser* pHttpParser, char* pBuf, size_t len) {
  //pHttpParser->data->state = PARSER_STATE_BODY_COMPLETE;
  IotHttpsReturnCode_t ret;
  return ret;
}


IotNetworkError_t IotNetworkInterfaceCreate( void * pConnectionInfo,
					     void * pCredentialInfo,
					     void * pConnection )
{
  //__CPROVER_assert( pConnectionInfo, "First argument nonnull");
  //__CPROVER_assert( pConnection, "Third argument nonnull" );

  *(char **)pConnection = malloc(1); // network connection is opaque
  IotNetworkError_t error;
  return error;
}

size_t IotNetworkInterfaceSend( void * pConnection,
			     const uint8_t * pMessage,
			     size_t messageLength )
{
  //__CPROVER_assert( pConnection, "First argument nonnull" );
  //__CPROVER_assert( pMessage, "Second argument nonnull" );
  // check buffer contains as much data as claimed
  //__CPROVER_assert(__CPROVER_r_ok(pMessage, messageLength),
		//   "Message readable");

  size_t size;
  __CPROVER_assume(size <= messageLength);
  return size;
}

IotNetworkError_t IotNetworkInterfaceClose( void * pConnection )
{
  //__CPROVER_assert( pConnection, "Argument nonnull" ); // and valid?

  IotNetworkError_t error;
  return error;
}

size_t IotNetworkInterfaceReceive( void * pConnection,
				uint8_t * pBuffer,
				size_t bytesRequested )
{
  //__CPROVER_assert( pConnection, "First argument nonnull" );
  //__CPROVER_assert( pBuffer, "Second argument nonnull" );
  // check buffer can hold as much data as requested
  //__CPROVER_assert(__CPROVER_w_ok(pBuffer, bytesRequested),
		 //  "Buffer readable");

  // fill buffer with unconstrained data
  uint8_t byte;
  __CPROVER_array_copy(pBuffer,&byte);

  size_t size;
  __CPROVER_assume(size <= bytesRequested);
  return size;
}

IotNetworkError_t IotNetworkInterfaceCallback( void * pConnection,
					       IotNetworkReceiveCallback_t
					       receiveCallback,
					       void * pContext )
{
  //__CPROVER_assert( pConnection, "First argument nonnull");       // and valid?
  //__CPROVER_assert( receiveCallback, "Second argument nonnull" ); // and valid?
  //__CPROVER_assert( pContext, "Third argument nonnull" );         // and valid?

  IotNetworkError_t error;
  return error;
}

IotNetworkError_t IotNetworkInterfaceDestroy( void * pConnection )
{
  //__CPROVER_assert( pConnection, "Argument nonnull" ); // and valid?

  IotNetworkError_t error;
  return error;
}

IotNetworkInterface_t IOTNI = {
  .create = IotNetworkInterfaceCreate,
  .close = IotNetworkInterfaceClose,
  .send = IotNetworkInterfaceSend,
  .receive = IotNetworkInterfaceReceive,
  .setReceiveCallback = IotNetworkInterfaceCallback,
  .destroy = IotNetworkInterfaceDestroy
};

IotNetworkInterface_t *newNetworkInterface() {
  return &IOTNI;
}

IotHttpsConnectionInfo_t * newConnectionInfo() {
  IotHttpsConnectionInfo_t * pConnInfo = malloc(sizeof(IotHttpsConnectionInfo_t));
  pConnInfo->pNetworkInterface = newNetworkInterface();
  pConnInfo->pNetworkServerInfo = malloc(sizeof(IotNetworkServerInfo_t));
  pConnInfo->pNetworkCredentialInfo = malloc(sizeof(IotNetworkCredentials_t));
  pConnInfo->pAddress = malloc(sizeof(uint32_t));
  pConnInfo->pAlpnProtocols = malloc(sizeof(uint32_t));
  pConnInfo->pCaCert = malloc(sizeof(uint32_t));
  pConnInfo->pClientCert = malloc(sizeof(uint32_t));
  pConnInfo->pPrivateKey = malloc(sizeof(uint32_t));
  pConnInfo->userBuffer.pBuffer = malloc(sizeof(struct _httpsConnection));
  return pConnInfo;
}

IotHttpsConnectionHandle_t newIotConnectionHandle () {
	IotHttpsConnectionHandle_t pConnectionHandle =  malloc(sizeof(struct _httpsConnection));
	pConnectionHandle->pNetworkConnection = malloc(sizeof(struct _httpsConnection));
	pConnectionHandle->pNetworkInterface = newNetworkInterface();
	return pConnectionHandle;
}

IotHttpsRequestHandle_t newIotRequestHandle() {
	IotHttpsRequestHandle_t requestHandle = malloc(sizeof(struct _httpsRequest));
	requestHandle->pRespHandle = malloc(sizeof(struct _httpsResponse));
	requestHandle->pConnHandle = malloc(sizeof(struct _httpsConnection));
	requestHandle->pBody = malloc(sizeof(uint32_t));
  requestHandle->pHeaders = malloc(sizeof(uint32_t));
	requestHandle->pConnInfo = newConnectionInfo();
	return requestHandle;
}

IotHttpsResponseHandle_t newIotResponseHandle() {
	IotHttpsResponseHandle_t pResponseHandle = malloc(sizeof(struct _httpsResponse));
	pResponseHandle->pBody = malloc(sizeof(uint32_t));
	pResponseHandle->pConnHandle = malloc(sizeof(struct _httpsConnection));
    pResponseHandle->pReqHandle = malloc(sizeof(struct _httpsRequest));
    pResponseHandle->httpParser.data = malloc(sizeof(struct _httpsResponse));
	return pResponseHandle;
}

void harness() {
	IotHttpsConnectionHandle_t pConnHandle = newIotConnectionHandle();
	IotHttpsRequestHandle_t reqHandle = newIotRequestHandle();
	IotHttpsResponseHandle_t pRespHandle = newIotResponseHandle();
	http_parser * pHttpParser = malloc(sizeof(http_parser));
	IotHttpsClient_SendSync(&pConnHandle, reqHandle, &pRespHandle);
}
