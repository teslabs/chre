/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "chpp/clients/wifi.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chpp/app.h"
#include "chpp/clients.h"
#include "chpp/clients/discovery.h"
#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
#include "chpp/clients/timesync.h"
#endif
#include "chpp/common/standard_uuids.h"
#include "chpp/common/wifi.h"
#include "chpp/common/wifi_types.h"
#include "chpp/log.h"
#include "chpp/macros.h"
#include "chpp/memory.h"
#include "chre/pal/wifi.h"
#include "chre_api/chre/wifi.h"

#ifndef CHPP_WIFI_DISCOVERY_TIMEOUT_MS
#define CHPP_WIFI_DISCOVERY_TIMEOUT_MS CHPP_DISCOVERY_DEFAULT_TIMEOUT_MS
#endif

#ifndef CHPP_WIFI_MAX_TIMESYNC_AGE_NS
#define CHPP_WIFI_MAX_TIMESYNC_AGE_NS CHPP_TIMESYNC_DEFAULT_MAX_AGE_NS
#endif

/************************************************
 *  Prototypes
 ***********************************************/

static enum ChppAppErrorCode chppDispatchWifiResponse(void *clientContext,
                                                      uint8_t *buf, size_t len);
static enum ChppAppErrorCode chppDispatchWifiNotification(void *clientContext,
                                                          uint8_t *buf,
                                                          size_t len);
static bool chppWifiClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion);
static void chppWifiClientDeinit(void *clientContext);
static void chppWifiClientNotifyReset(void *clientContext);

/************************************************
 *  Private Definitions
 ***********************************************/

/**
 * Configuration parameters for this client
 */
static const struct ChppClient kWifiClientConfig = {
    .descriptor.uuid = CHPP_UUID_WIFI_STANDARD,

    // Version
    .descriptor.version.major = 1,
    .descriptor.version.minor = 0,
    .descriptor.version.patch = 0,

    // Notifies client if CHPP is reset
    .resetNotifierFunctionPtr = &chppWifiClientNotifyReset,

    // Service response dispatch function pointer
    .responseDispatchFunctionPtr = &chppDispatchWifiResponse,

    // Service notification dispatch function pointer
    .notificationDispatchFunctionPtr = &chppDispatchWifiNotification,

    // Service response dispatch function pointer
    .initFunctionPtr = &chppWifiClientInit,

    // Service notification dispatch function pointer
    .deinitFunctionPtr = &chppWifiClientDeinit,

    // Min length is the entire header
    .minLength = sizeof(struct ChppAppHeader),
};

/**
 * Structure to maintain state for the WiFi client and its Request/Response
 * (RR) functionality.
 */
struct ChppWifiClientState {
  struct ChppClientState client;     // WiFi client state
  const struct chrePalWifiApi *api;  // WiFi PAL API

  struct ChppRequestResponseState open;             // Service init state
  struct ChppRequestResponseState close;            // Service deinit state
  struct ChppRequestResponseState getCapabilities;  // Get Capabilities state
  struct ChppRequestResponseState
      configureScanMonitor;                     // Configure Scan Monitor state
  struct ChppRequestResponseState requestScan;  // Request Scan state
  struct ChppRequestResponseState requestRanging;  // Request Ranging state

  uint32_t capabilities;  // Cached GetCapabilities result

  bool scanMonitorEnabled;          // Scan monitoring is enabled
  bool scanMonitorSilenceCallback;  // Silence callback during recovery from a
                                    // service reset
};

// Note: This global definition of gWifiClientContext supports only one
// instance of the CHPP WiFi client at a time.
struct ChppWifiClientState gWifiClientContext;
static const struct chrePalSystemApi *gSystemApi;
static const struct chrePalWifiCallbacks *gCallbacks;

/************************************************
 *  Prototypes
 ***********************************************/

static bool chppWifiClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWifiCallbacks *callbacks);
static void chppWifiClientClose(void);
static uint32_t chppWifiClientGetCapabilities(void);
static bool chppWifiClientConfigureScanMonitor(bool enable);
static bool chppWifiClientRequestScan(const struct chreWifiScanParams *params);
static void chppWifiClientReleaseScanEvent(struct chreWifiScanEvent *event);
static bool chppWifiClientRequestRanging(
    const struct chreWifiRangingParams *params);
static void chppWifiClientReleaseRangingEvent(
    struct chreWifiRangingEvent *event);

static void chppWiFiRecoverScanMonitor(
    struct ChppWifiClientState *clientContext);
static void chppWifiCloseResult(struct ChppWifiClientState *clientContext,
                                uint8_t *buf, size_t len);
static void chppWifiGetCapabilitiesResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);
static void chppWifiConfigureScanMonitorResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);
static void chppWifiRequestScanResult(struct ChppWifiClientState *clientContext,
                                      uint8_t *buf, size_t len);

static void chppWifiScanEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);
static void chppWifiRangingEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len);

/************************************************
 *  Private Functions
 ***********************************************/

/**
 * Dispatches a service response from the transport layer that is determined to
 * be for the WiFi client.
 *
 * This function is called from the app layer using its function pointer given
 * during client registration.
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppDispatchWifiResponse(void *clientContext,
                                                      uint8_t *buf,
                                                      size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  switch (rxHeader->command) {
    case CHPP_WIFI_OPEN: {
      chppClientTimestampResponse(&wifiClientContext->open, rxHeader);
      chppClientProcessOpenResponse(&wifiClientContext->client, buf, len);
      chppWiFiRecoverScanMonitor(wifiClientContext);
      break;
    }

    case CHPP_WIFI_CLOSE: {
      chppClientTimestampResponse(&wifiClientContext->close, rxHeader);
      chppWifiCloseResult(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_GET_CAPABILITIES: {
      chppClientTimestampResponse(&wifiClientContext->getCapabilities,
                                  rxHeader);
      chppWifiGetCapabilitiesResult(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC: {
      chppClientTimestampResponse(&wifiClientContext->configureScanMonitor,
                                  rxHeader);
      chppWifiConfigureScanMonitorResult(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_REQUEST_SCAN_ASYNC: {
      chppClientTimestampResponse(&wifiClientContext->requestScan, rxHeader);
      chppWifiRequestScanResult(wifiClientContext, buf, len);
      break;
    }

    default: {
      error = CHPP_APP_ERROR_INVALID_COMMAND;
      break;
    }
  }

  return error;
}

/**
 * Dispatches a service notification from the transport layer that is determined
 * to be for the WiFi client.
 *
 * This function is called from the app layer using its function pointer given
 * during client registration.
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 *
 * @return Indicates the result of this function call.
 */
static enum ChppAppErrorCode chppDispatchWifiNotification(void *clientContext,
                                                          uint8_t *buf,
                                                          size_t len) {
  struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  enum ChppAppErrorCode error = CHPP_APP_ERROR_NONE;

  switch (rxHeader->command) {
    case CHPP_WIFI_REQUEST_SCAN_ASYNC: {
      chppWifiScanEventNotification(wifiClientContext, buf, len);
      break;
    }

    case CHPP_WIFI_REQUEST_RANGING_ASYNC: {
      chppWifiRangingEventNotification(wifiClientContext, buf, len);
      break;
    }

    default: {
      error = CHPP_APP_ERROR_INVALID_COMMAND;
      break;
    }
  }

  return error;
}

/**
 * Initializes the client and provides its handle number and the version of the
 * matched service when/if it the client is matched with a service during
 * discovery.
 *
 * @param clientContext Maintains status for each client instance.
 * @param handle Handle number for this client.
 * @param serviceVersion Version of the matched service.
 *
 * @return True if client is compatible and successfully initialized.
 */
static bool chppWifiClientInit(void *clientContext, uint8_t handle,
                               struct ChppVersion serviceVersion) {
  UNUSED_VAR(serviceVersion);

  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  chppClientInit(&wifiClientContext->client, handle);

  return true;
}

/**
 * Deinitializes the client.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWifiClientDeinit(void *clientContext) {
  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;
  chppClientDeinit(&wifiClientContext->client);
}

/**
 * Notifies the client of an incoming reset.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWifiClientNotifyReset(void *clientContext) {
  struct ChppWifiClientState *wifiClientContext =
      (struct ChppWifiClientState *)clientContext;

  if (wifiClientContext->client.openState != CHPP_OPEN_STATE_OPENED) {
    CHPP_LOGW("WiFi client reset but client wasn't open");
  } else {
    wifiClientContext->client.openState = CHPP_OPEN_STATE_REOPENING;
    chppWifiClientOpen(gSystemApi, gCallbacks);
  }
}

/**
 * Restores the state of scan monitoring after an incoming reset.
 *
 * @param clientContext Maintains status for each client instance.
 */
static void chppWiFiRecoverScanMonitor(
    struct ChppWifiClientState *clientContext) {
  if (clientContext->scanMonitorEnabled) {
    CHPP_LOGI("Re-enabling WiFi scan monitoring after reset");
    clientContext->scanMonitorEnabled = false;
    clientContext->scanMonitorSilenceCallback = true;

    if (!chppWifiClientConfigureScanMonitor(true)) {
      clientContext->scanMonitorSilenceCallback = false;
      CHPP_LOGE("Unable to re-enable WiFi scan monitoring after reset");
      CHPP_PROD_ASSERT(false);
    }
  }
}

/**
 * Handles the service response for the close client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiCloseResult(struct ChppWifiClientState *clientContext,
                                uint8_t *buf, size_t len) {
  // TODO
  UNUSED_VAR(clientContext);
  UNUSED_VAR(buf);
  UNUSED_VAR(len);
}

/**
 * Handles the service response for the get capabilities client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiGetCapabilitiesResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  if (len < sizeof(struct ChppWifiGetCapabilitiesResponse)) {
    struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
    CHPP_LOGE("WiFi GetCapabilities request failed at service. error=%" PRIu8,
              rxHeader->error);

  } else {
    struct ChppWifiGetCapabilitiesParameters *result =
        &((struct ChppWifiGetCapabilitiesResponse *)buf)->params;

    CHPP_LOGI("chppWifiGetCapabilitiesResult received capabilities=0x%" PRIx32,
              result->capabilities);
#ifdef CHPP_WIFI_DEFAULT_CAPABILITIES
    if (result->capabilities != CHPP_WIFI_DEFAULT_CAPABILITIES) {
      CHPP_LOGE("Unexpected capability: expected 0x%" PRIx32,
                CHPP_WIFI_DEFAULT_CAPABILITIES);
      CHPP_PROD_ASSERT(false);
    }
#endif

    clientContext->capabilities = result->capabilities;
  }
}

/**
 * Handles the service response for the Configure Scan Monitor client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiConfigureScanMonitorResult(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);

  if (len < sizeof(struct ChppWifiConfigureScanMonitorAsyncResponse)) {
    struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
    CHPP_LOGE(
        "WiFi ControlLocationSession request failed at service. "
        "error=%" PRIu8,
        rxHeader->error);

  } else {
    struct ChppWifiConfigureScanMonitorAsyncResponseParameters *result =
        &((struct ChppWifiConfigureScanMonitorAsyncResponse *)buf)->params;

    gWifiClientContext.scanMonitorEnabled = result->enabled;
    CHPP_LOGI(
        "chppWifiConfigureScanMonitorResult received enable=%s, "
        "errorCode=%" PRIu8,
        result->enabled ? "true" : "false", result->errorCode);

    if (!gWifiClientContext.scanMonitorSilenceCallback) {
      // Per the scanMonitorStatusChangeCallback API contract, unsolicited
      // calls to scanMonitorStatusChangeCallback must not be made, and it
      // should only be invoked as the direct result of an earlier call to
      // configureScanMonitor.
      gCallbacks->scanMonitorStatusChangeCallback(result->enabled,
                                                  result->errorCode);
    }  // Else, the WiFi subsystem has been reset and we are required to
       // silently reenable the scan monitor.

    gWifiClientContext.scanMonitorSilenceCallback = false;
  }
}

/**
 * Handles the service response for the Request Scan Result client request.
 *
 * This function is called from chppDispatchWifiResponse().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiRequestScanResult(struct ChppWifiClientState *clientContext,
                                      uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);

  if (len < sizeof(struct ChppWifiRequestScanResponse)) {
    struct ChppAppHeader *rxHeader = (struct ChppAppHeader *)buf;
    CHPP_LOGE("WiFi RequestScanResult request failed at service. error=%" PRIu8,
              rxHeader->error);

  } else {
    struct ChppWifiRequestScanResponseParameters *result =
        &((struct ChppWifiRequestScanResponse *)buf)->params;

    CHPP_LOGI("WiFi RequestScanResult request %ssuccessful at service",
              result->pending ? "accepted and " : "FAILURE - accepted but un");

    gCallbacks->scanResponseCallback(result->pending, result->errorCode);
  }
}

/**
 * Handles the WiFi scan event service notification.
 *
 * This function is called from chppDispatchWifiNotification().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiScanEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);
  CHPP_LOGI("chppWifiScanEventNotification received data len=%" PRIuSIZE, len);

  buf += sizeof(struct ChppAppHeader);
  len -= sizeof(struct ChppAppHeader);

  struct chreWifiScanEvent *chre =
      chppWifiScanEventToChre((struct ChppWifiScanEvent *)buf, len);

  if (chre == NULL) {
    CHPP_LOGE(
        "chppWifiScanEventNotification CHPP -> CHRE conversion failed. Input "
        "len=%" PRIuSIZE,
        len);
  } else {
#ifdef CHPP_CLIENT_ENABLED_TIMESYNC
    chre->referenceTime -= (uint64_t)chppTimesyncGetOffset(
        gWifiClientContext.client.appContext, CHPP_WIFI_MAX_TIMESYNC_AGE_NS);
#endif

    gCallbacks->scanEventCallback(chre);
  }
}

/**
 * Handles the WiFi ranging event service notification.
 *
 * This function is called from chppDispatchWifiNotification().
 *
 * @param clientContext Maintains status for each client instance.
 * @param buf Input data. Cannot be null.
 * @param len Length of input data in bytes.
 */
static void chppWifiRangingEventNotification(
    struct ChppWifiClientState *clientContext, uint8_t *buf, size_t len) {
  UNUSED_VAR(clientContext);

  CHPP_LOGD("chppWifiRangingEventNotification received data len=%" PRIuSIZE,
            len);

  buf += sizeof(struct ChppAppHeader);
  len -= sizeof(struct ChppAppHeader);

  struct chreWifiRangingEvent *chre =
      chppWifiRangingEventToChre((struct ChppWifiRangingEvent *)buf, len);

  uint8_t error = CHRE_ERROR_NONE;
  if (chre == NULL) {
    error = CHRE_ERROR;
    CHPP_LOGE(
        "chppWifiRangingEventNotification CHPP -> CHRE conversion failed. "
        "Input len=%" PRIuSIZE,
        len);
  }

  gCallbacks->rangingEventCallback(error, chre);
}

/**
 * Initializes the WiFi client upon an open request from CHRE and responds
 * with the result.
 *
 * @param systemApi CHRE system function pointers.
 * @param callbacks CHRE entry points.
 *
 * @return True if successful. False otherwise.
 */
static bool chppWifiClientOpen(const struct chrePalSystemApi *systemApi,
                               const struct chrePalWifiCallbacks *callbacks) {
  CHPP_DEBUG_ASSERT(systemApi != NULL);
  CHPP_DEBUG_ASSERT(callbacks != NULL);

  bool result = false;
  gSystemApi = systemApi;
  gCallbacks = callbacks;

  CHPP_LOGI("WiFi client %sopening (openState=%" PRIu8 ")",
            (gWifiClientContext.client.openState == CHPP_OPEN_STATE_REOPENING)
                ? "re"
                : "",
            gWifiClientContext.client.openState);

  if (gWifiClientContext.client.openState == CHPP_OPEN_STATE_CLOSED) {
    gWifiClientContext.capabilities = CHRE_WIFI_CAPABILITIES_NONE;
  }

  // Wait for discovery to complete for "open" call to succeed
  if (!chppWaitForDiscoveryComplete(gWifiClientContext.client.appContext,
                                    CHPP_WIFI_DISCOVERY_TIMEOUT_MS)) {
    CHPP_LOGE("Timed out waiting to discover CHPP WiFi service");
  } else {
    result = chppClientSendOpenRequest(
        &gWifiClientContext.client, &gWifiClientContext.open, CHPP_WIFI_OPEN);
  }

#ifdef CHPP_WIFI_CLIENT_OPEN_ALWAYS_SUCCESS
  result = true;
#endif

  return result;
}

/**
 * Deinitializes the WiFi client.
 */
static void chppWifiClientClose(void) {
  // Remote
  struct ChppAppHeader *request = chppAllocClientRequestCommand(
      &gWifiClientContext.client, CHPP_WIFI_CLOSE);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else if (chppSendTimestampedRequestAndWait(&gWifiClientContext.client,
                                               &gWifiClientContext.close,
                                               request, sizeof(*request))) {
    gWifiClientContext.client.openState = CHPP_OPEN_STATE_CLOSED;
    gWifiClientContext.capabilities = CHRE_WIFI_CAPABILITIES_NONE;
  }
}

/**
 * Retrieves a set of flags indicating the WiFi features supported by the
 * current implementation.
 *
 * @return Capabilities flags.
 */
static uint32_t chppWifiClientGetCapabilities(void) {
#ifdef CHPP_WIFI_DEFAULT_CAPABILITIES
  uint32_t capabilities = CHPP_WIFI_DEFAULT_CAPABILITIES;
#else
  uint32_t capabilities = CHRE_WIFI_CAPABILITIES_NONE;
#endif

  if (gWifiClientContext.capabilities != CHRE_WIFI_CAPABILITIES_NONE) {
    // Result already cached
    capabilities = gWifiClientContext.capabilities;

  } else {
    struct ChppAppHeader *request = chppAllocClientRequestCommand(
        &gWifiClientContext.client, CHPP_WIFI_GET_CAPABILITIES);

    if (request == NULL) {
      CHPP_LOG_OOM();
    } else {
      if (chppSendTimestampedRequestAndWait(&gWifiClientContext.client,
                                            &gWifiClientContext.getCapabilities,
                                            request, sizeof(*request))) {
        // Success. gWifiClientContext.capabilities is now populated
        capabilities = gWifiClientContext.capabilities;
      }
    }
  }

  return capabilities;
}

/**
 * Enables/disables receiving unsolicited scan results (scan monitoring).
 *
 * @param enable True to enable.
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWifiClientConfigureScanMonitor(bool enable) {
  bool result = false;

  struct ChppWifiConfigureScanMonitorAsyncRequest *request =
      chppAllocClientRequestFixed(
          &gWifiClientContext.client,
          struct ChppWifiConfigureScanMonitorAsyncRequest);

  if (request == NULL) {
    CHPP_LOG_OOM();
  } else {
    request->header.command = CHPP_WIFI_CONFIGURE_SCAN_MONITOR_ASYNC;
    request->params.enable = enable;
    request->params.cookie = &gWifiClientContext.configureScanMonitor;

    result = chppSendTimestampedRequestOrFail(
        &gWifiClientContext.client, &gWifiClientContext.configureScanMonitor,
        request, sizeof(*request));
  }

  return result;
}

/**
 * Request that the WiFi chipset perform a scan or deliver results from its
 * cache.
 *
 * @param params See chreWifiRequestScanAsync().
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWifiClientRequestScan(const struct chreWifiScanParams *params) {
  struct ChppWifiScanParamsWithHeader *request;
  size_t requestLen;

  bool result = chppWifiScanParamsFromChre(params, &request, &requestLen);

  if (!result) {
    CHPP_LOG_OOM();
  } else {
    request->header.handle = gWifiClientContext.client.handle;
    request->header.type = CHPP_MESSAGE_TYPE_CLIENT_REQUEST;
    request->header.transaction = gWifiClientContext.client.transaction++;
    request->header.error = CHPP_APP_ERROR_NONE;
    request->header.command = CHPP_WIFI_REQUEST_SCAN_ASYNC;

    result = chppSendTimestampedRequestOrFail(&gWifiClientContext.client,
                                              &gWifiClientContext.requestScan,
                                              request, requestLen);
  }

  return result;
}

/**
 * Releases the memory held for the scan event callback.
 *
 * @param event Location event to be released.
 */
static void chppWifiClientReleaseScanEvent(struct chreWifiScanEvent *event) {
  void *scannedFreqList = CHPP_CONST_CAST_POINTER(event->scannedFreqList);
  CHPP_FREE_AND_NULLIFY(scannedFreqList);
  void *results = CHPP_CONST_CAST_POINTER(event->results);
  CHPP_FREE_AND_NULLIFY(results);
  CHPP_FREE_AND_NULLIFY(event);
}

/**
 * Request that the WiFi chipset perform RTT ranging.
 *
 * @param params See chreWifiRequestRangingAsync().
 *
 * @return True indicates the request was sent off to the service.
 */
static bool chppWifiClientRequestRanging(
    const struct chreWifiRangingParams *params) {
  struct ChppWifiRangingParamsWithHeader *request;
  size_t requestLen;

  bool result = chppWifiRangingParamsFromChre(params, &request, &requestLen);

  if (!result) {
    CHPP_LOG_OOM();
  } else {
    request->header.handle = gWifiClientContext.client.handle;
    request->header.type = CHPP_MESSAGE_TYPE_CLIENT_REQUEST;
    request->header.transaction = gWifiClientContext.client.transaction++;
    request->header.error = CHPP_APP_ERROR_NONE;
    request->header.command = CHPP_WIFI_REQUEST_RANGING_ASYNC;

    result = chppSendTimestampedRequestOrFail(
        &gWifiClientContext.client, &gWifiClientContext.requestRanging, request,
        requestLen);
  }

  return result;
}

/**
 * Releases the memory held for the RTT ranging event callback.
 *
 * @param event Location event to be released.
 */
static void chppWifiClientReleaseRangingEvent(
    struct chreWifiRangingEvent *event) {
  void *results = CHPP_CONST_CAST_POINTER(event->results);
  CHPP_FREE_AND_NULLIFY(results);
  CHPP_FREE_AND_NULLIFY(event);
}

/************************************************
 *  Public Functions
 ***********************************************/

void chppRegisterWifiClient(struct ChppAppState *appContext) {
  gWifiClientContext.client.appContext = appContext;
  chppRegisterClient(appContext, (void *)&gWifiClientContext,
                     &kWifiClientConfig);
}

void chppDeregisterWifiClient(struct ChppAppState *appContext) {
  // TODO

  UNUSED_VAR(appContext);
}

#ifdef CHPP_CLIENT_ENABLED_WIFI

#ifdef CHPP_CLIENT_ENABLED_CHRE_WIFI
const struct chrePalWifiApi *chrePalWifiGetApi(uint32_t requestedApiVersion) {
#else
const struct chrePalWifiApi *chppPalWifiGetApi(uint32_t requestedApiVersion) {
#endif

  static const struct chrePalWifiApi api = {
      .moduleVersion = CHPP_PAL_WIFI_API_VERSION,
      .open = chppWifiClientOpen,
      .close = chppWifiClientClose,
      .getCapabilities = chppWifiClientGetCapabilities,
      .configureScanMonitor = chppWifiClientConfigureScanMonitor,
      .requestScan = chppWifiClientRequestScan,
      .releaseScanEvent = chppWifiClientReleaseScanEvent,
      .requestRanging = chppWifiClientRequestRanging,
      .releaseRangingEvent = chppWifiClientReleaseRangingEvent,
  };

  CHPP_STATIC_ASSERT(
      CHRE_PAL_WIFI_API_CURRENT_VERSION == CHPP_PAL_WIFI_API_VERSION,
      "A newer CHRE PAL API version is available. Please update.");

  if (!CHRE_PAL_VERSIONS_ARE_COMPATIBLE(api.moduleVersion,
                                        requestedApiVersion)) {
    return NULL;
  } else {
    return &api;
  }
}

#endif
