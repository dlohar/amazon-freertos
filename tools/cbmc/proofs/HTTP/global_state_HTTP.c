#define MAX_DATA_SIZE 1000

/* Implementation of safe malloc which returns NULL if the requested size is 0.
 Depending on the platform the malloc may return either NULL or an address. */
void *safeMalloc(size_t xWantedSize) {
  if(xWantedSize == 0) {
    return NULL;
  }
  uint8_t byte;
  return byte ? malloc(xWantedSize) : NULL;
}

/* It is common for a buffer to contain a header struct followed by user data.
We optimize CBMC performance by allocating space for the buffer using a
struct with two members: the first member is the header struct and the
second member is the user data. This is faster than just allocating a
sequence of bytes large enough to hold the header struct and the user data.
We modeled responseHandle, requestHandle and connectionHandle similarly. */
typedef struct _responseHandle
{
  struct _httpsResponse RespHandle;
  char data[MAX_DATA_SIZE];
} _resHandle_t;

typedef struct _requestHandle
{
  struct _httpsRequest ReqHandle;
  char data[MAX_DATA_SIZE];
} _reqHandle_t;

typedef struct _connectionHandle
{
  struct _httpsConnection pConnHandle;
  char data[MAX_DATA_SIZE];
} _connHandle_t;

/* Models the third party HTTP Parser. */
size_t http_parser_execute (http_parser *parser,
                            const http_parser_settings *settings,
                            const char *data,
                            size_t len) {
  size_t ret;
  _httpsResponse_t *_httpsResponse = (_httpsResponse_t *)(parser->data);
  /* nondet_bool is required to get coverage. */
  _httpsResponse->foundHeaderField = nondet_bool();
  _httpsResponse->parserState = PARSER_STATE_BODY_COMPLETE;
  return ret;
}

IotNetworkError_t IotNetworkInterfaceCreate( void * pConnectionInfo,
               void * pCredentialInfo,
               void * pConnection ) {
  *(char **)pConnection = malloc(1); /* network connection is opaque */
  IotNetworkError_t error;
  return error;
}

size_t IotNetworkInterfaceSend( void * pConnection,
           const uint8_t * pMessage,
           size_t messageLength ) {
  size_t size;
  __CPROVER_assume(size <= messageLength);
  return size;
}

IotNetworkError_t IotNetworkInterfaceClose( void * pConnection ) {
  IotNetworkError_t error;
  return error;
}

size_t IotNetworkInterfaceReceive( void * pConnection,
        uint8_t * pBuffer,
        size_t bytesRequested ) {
  uint8_t byte;
  /* This intrinsic copies a source to a destination, and fills the
  remainder of the destination with unconstrained data.*/
  __CPROVER_array_copy(pBuffer,&byte);

  size_t size;
  __CPROVER_assume(size <= bytesRequested);
  return size;
}

IotNetworkError_t IotNetworkInterfaceCallback( void * pConnection,
                 IotNetworkReceiveCallback_t
                 receiveCallback,
                 void * pContext ) {
  IotNetworkError_t error;
  return error;
}

IotNetworkError_t IotNetworkInterfaceDestroy( void * pConnection ) {
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

/* Models the Network Interface. */
IotNetworkInterface_t *newNetworkInterface() {
  return &IOTNI;
}

/* Creates a Connection Info and assigns memory accordingly. */
IotHttpsConnectionInfo_t * newConnectionInfo() {
  IotHttpsConnectionInfo_t * pConnInfo = safeMalloc(sizeof(IotHttpsConnectionInfo_t));
  if(pConnInfo) {
    pConnInfo->pNetworkInterface = newNetworkInterface();
    /* Function _createHttpsConnection checks if pConnInfo->addressLen is less than
    IOT_HTTPS_MAX_HOST_NAME_LENGTH. The +1 is added to cover all cases. */
    __CPROVER_assume(pConnInfo->addressLen >= 0 && pConnInfo->addressLen <= IOT_HTTPS_MAX_HOST_NAME_LENGTH + 1);
    pConnInfo->pAddress = safeMalloc(pConnInfo->addressLen);
    /* Function _createHttpsConnection checks if pConnInfo->alpnProtocolsLen is less than
    IOT_HTTPS_MAX_ALPN_PROTOCOLS_LENGTH. The +1 is added to cover all cases. */
    __CPROVER_assume(pConnInfo->alpnProtocolsLen >= 0 && pConnInfo->alpnProtocolsLen <= IOT_HTTPS_MAX_ALPN_PROTOCOLS_LENGTH + 1);
    pConnInfo->pAlpnProtocols = safeMalloc(pConnInfo->alpnProtocolsLen);
    pConnInfo->pCaCert = malloc(sizeof(uint32_t));
    pConnInfo->pClientCert = malloc(sizeof(uint32_t));
    pConnInfo->pPrivateKey = malloc(sizeof(uint32_t));
    pConnInfo->userBuffer.pBuffer = safeMalloc(sizeof(_connHandle_t));
  }
  return pConnInfo;
}

/* Creates a Connection Handle and assigns memory accordingly. */
IotHttpsConnectionHandle_t newIotConnectionHandle () {
  IotHttpsConnectionHandle_t pConnectionHandle = safeMalloc(sizeof(_connHandle_t));
  if(pConnectionHandle) {
    pConnectionHandle->pNetworkConnection = safeMalloc(sizeof(uint32_t));
    pConnectionHandle->pNetworkInterface = newNetworkInterface();
    pConnectionHandle->reqQ.pPrevious = NULL;
    pConnectionHandle->reqQ.pNext = NULL;
    pConnectionHandle->respQ.pPrevious = NULL;
    pConnectionHandle->respQ.pNext = NULL;
    //pConnectionHandle->taskPoolJob = malloc(sizeof(struct _taskPoolJob));
  }
  return pConnectionHandle;
}

/* Creates a Response Handle and assigns memory accordingly. */
IotHttpsResponseHandle_t newIotResponseHandle() {
  IotHttpsResponseHandle_t pResponseHandle = safeMalloc(sizeof(_resHandle_t));
  if(pResponseHandle) {
    pResponseHandle->pBody = malloc(sizeof(uint32_t));
    pResponseHandle->pHttpsConnection = newIotConnectionHandle();
    pResponseHandle->pHttpsRequest = safeMalloc(sizeof(_reqHandle_t));
    if(pResponseHandle->pHttpsRequest) {
      pResponseHandle->pHttpsRequest->pHttpsResponse = pResponseHandle;
    }
    pResponseHandle->pHeaders = safeMalloc(sizeof(pResponseHandle) + sizeof(struct _httpResponse));
    pResponseHandle->pHeadersCur = pResponseHandle->pHeaders;
    pResponseHandle->httpParserInfo.readHeaderParser.data = pResponseHandle;
    pResponseHandle->httpParserInfo.parseFunc = http_parser_execute;
    pResponseHandle->pReadHeaderValue = safeMalloc(sizeof(uint32_t));
    pResponseHandle->pHeadersEnd = safeMalloc(sizeof(pResponseHandle) + sizeof(_resHandle_t)); ;
    __CPROVER_assume(pResponseHandle->readHeaderValueLength >= 0 &&
    pResponseHandle->readHeaderValueLength <= (pResponseHandle->pHeadersEnd - pResponseHandle->pHeaders));
    pResponseHandle->pReadHeaderValue = malloc(pResponseHandle->readHeaderValueLength);
  }
  return pResponseHandle;
}

/* Creates a Request Handle and assigns memory accordingly. */
IotHttpsRequestHandle_t newIotRequestHandle() {
  IotHttpsRequestHandle_t pRequestHandle = safeMalloc(sizeof(_reqHandle_t));
  if (pRequestHandle) {
    uint32_t len;
    pRequestHandle->pHttpsResponse = newIotResponseHandle();
    pRequestHandle->pHttpsConnection = newIotConnectionHandle();
    pRequestHandle->pBody = malloc(len);
    pRequestHandle->pHeaders = safeMalloc(sizeof(pRequestHandle) + sizeof(struct _httpRequest));
    pRequestHandle->pHeadersCur = pRequestHandle->pHeaders;
    pRequestHandle->pHeadersEnd = safeMalloc(sizeof(pRequestHandle) + sizeof(_reqHandle_t));
    pRequestHandle->pConnInfo = newConnectionInfo();
  }
  return pRequestHandle;
}

/* Creates a Request Info and assigns memory accordingly. */
IotHttpsRequestInfo_t * newIotRequestInfo() {
  IotHttpsRequestInfo_t * pReqInfo = safeMalloc(sizeof(IotHttpsRequestInfo_t));
  if(pReqInfo) {
    uint32_t bufferSize;
    uint32_t hostNameLen;
    __CPROVER_assume(bufferSize >=0 && bufferSize <= requestUserBufferMinimumSize);
    pReqInfo->userBuffer.bufferLen = bufferSize;
    pReqInfo->userBuffer.pBuffer = safeMalloc(bufferSize);
    __CPROVER_assume(hostNameLen >= 0 && hostNameLen <= IOT_HTTPS_MAX_HOST_NAME_LENGTH + 1);
    pReqInfo->pHost = safeMalloc(hostNameLen);
  }
  return pReqInfo;
}

/* Creates a Response Info and assigns memory accordingly. */
IotHttpsResponseInfo_t * newIotResponseInfo() {
  IotHttpsResponseInfo_t * pRespInfo = safeMalloc(sizeof(IotHttpsResponseInfo_t));
  if(pRespInfo) {
    uint32_t bufferSize;
    uint32_t bodySize;
    __CPROVER_assume(bufferSize > 0 && bufferSize <= responseUserBufferMinimumSize);
    pRespInfo->userBuffer.bufferLen = bufferSize;
    pRespInfo->userBuffer.pBuffer = malloc(bufferSize);
    pRespInfo->pSyncInfo = malloc(sizeof(IotHttpsSyncInfo_t));
    pRespInfo->pSyncInfo->pBody = malloc(bodySize);
    pRespInfo->pSyncInfo->bodyLen = bodySize;
  }
  return pRespInfo;
}
