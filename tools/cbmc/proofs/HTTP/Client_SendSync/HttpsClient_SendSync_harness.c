/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "FreeRTOS_Sockets.h"

/* FreeRTOS+TCP includes. */
#include "iot_https_client.h"
#include "iot_https_internal.h"

void harness() {
	IotHttpsConnectionHandle_t pConnHandle = malloc(sizeof(struct _httpsConnection));
	IotHttpsRequestHandle_t reqHandle = malloc(sizeof(struct _httpsRequest));
	IotHttpsResponseHandle_t pRespHandle = malloc(sizeof(struct _httpsResponse));
	IotHttpsClient_SendSync(&pConnHandle, reqHandle, &pRespHandle);
}
