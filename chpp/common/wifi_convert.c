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

// This file was automatically generated by chre_api_to_chpp.py
// Date: 2020-12-01 07:00:19 UTC
// Source: chre_api/include/chre_api/chre/wifi.h @ commit 5e7b258d

// DO NOT modify this file directly, as those changes will be lost the next
// time the script is executed

#include "chpp/common/wifi_types.h"
#include "chpp/macros.h"
#include "chpp/memory.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Encoding (CHRE --> CHPP) size functions

//! @return number of bytes required to represent the given
//! chreWifiScanEvent along with the CHPP header as
//! struct ChppWifiScanEventWithHeader
static size_t chppWifiSizeOfScanEventFromChre(
    const struct chreWifiScanEvent *scanEvent) {
  size_t encodedSize = sizeof(struct ChppWifiScanEventWithHeader);
  encodedSize += scanEvent->scannedFreqListLen * sizeof(uint32_t);
  encodedSize += scanEvent->resultCount * sizeof(struct ChppWifiScanResult);
  return encodedSize;
}

//! @return number of bytes required to represent the given
//! chreWifiScanParams along with the CHPP header as
//! struct ChppWifiScanParamsWithHeader
static size_t chppWifiSizeOfScanParamsFromChre(
    const struct chreWifiScanParams *scanParams) {
  size_t encodedSize = sizeof(struct ChppWifiScanParamsWithHeader);
  encodedSize += scanParams->frequencyListLen * sizeof(uint32_t);
  encodedSize += scanParams->ssidListLen * sizeof(struct ChppWifiSsidListItem);
  return encodedSize;
}

// Encoding (CHRE --> CHPP) conversion functions

static void chppWifiConvertScanResultFromChre(
    const struct chreWifiScanResult *in, struct ChppWifiScanResult *out) {
  out->ageMs = in->ageMs;
  out->capabilityInfo = in->capabilityInfo;
  out->ssidLen = in->ssidLen;
  memcpy(out->ssid, in->ssid, sizeof(out->ssid));
  memcpy(out->bssid, in->bssid, sizeof(out->bssid));
  out->flags = in->flags;
  out->rssi = in->rssi;
  out->band = in->band;
  out->primaryChannel = in->primaryChannel;
  out->centerFreqPrimary = in->centerFreqPrimary;
  out->centerFreqSecondary = in->centerFreqSecondary;
  out->channelWidth = in->channelWidth;
  out->securityMode = in->securityMode;
  out->radioChain = in->radioChain;
  out->rssiChain0 = in->rssiChain0;
  out->rssiChain1 = in->rssiChain1;
  memset(&out->reserved, 0, sizeof(out->reserved));
}

static void chppWifiConvertScanEventFromChre(const struct chreWifiScanEvent *in,
                                             struct ChppWifiScanEvent *out,
                                             uint8_t *payload,
                                             size_t payloadSize,
                                             uint16_t *vlaOffset) {
  out->version = CHRE_WIFI_SCAN_EVENT_VERSION;
  out->resultCount = in->resultCount;
  out->resultTotal = in->resultTotal;
  out->eventIndex = in->eventIndex;
  out->scanType = in->scanType;
  out->ssidSetSize = in->ssidSetSize;
  out->scannedFreqListLen = in->scannedFreqListLen;
  out->referenceTime = in->referenceTime;
  out->scannedFreqList.length = in->scannedFreqListLen * sizeof(uint32_t);
  CHPP_ASSERT((size_t)(*vlaOffset + out->scannedFreqList.length) <=
              payloadSize);
  if (out->scannedFreqList.length > 0 &&
      *vlaOffset + out->scannedFreqList.length <= payloadSize) {
    memcpy(&payload[*vlaOffset], in->scannedFreqList,
           in->scannedFreqListLen * sizeof(uint32_t));
    out->scannedFreqList.offset = *vlaOffset;
    *vlaOffset += out->scannedFreqList.length;
  } else {
    out->scannedFreqList.offset = 0;
  }

  struct ChppWifiScanResult *results =
      (struct ChppWifiScanResult *)&payload[*vlaOffset];
  out->results.length = in->resultCount * sizeof(struct ChppWifiScanResult);
  CHPP_ASSERT((size_t)(*vlaOffset + out->results.length) <= payloadSize);
  if (out->results.length > 0 &&
      *vlaOffset + out->results.length <= payloadSize) {
    for (size_t i = 0; i < in->resultCount; i++) {
      chppWifiConvertScanResultFromChre(&in->results[i], &results[i]);
    }
    out->results.offset = *vlaOffset;
    *vlaOffset += out->results.length;
  } else {
    out->results.offset = 0;
  }
  out->radioChainPref = in->radioChainPref;
}

static void chppWifiConvertSsidListItemFromChre(
    const struct chreWifiSsidListItem *in, struct ChppWifiSsidListItem *out) {
  out->ssidLen = in->ssidLen;
  memcpy(out->ssid, in->ssid, sizeof(out->ssid));
}

static void chppWifiConvertScanParamsFromChre(
    const struct chreWifiScanParams *in, struct ChppWifiScanParams *out,
    uint8_t *payload, size_t payloadSize, uint16_t *vlaOffset) {
  out->scanType = in->scanType;
  out->maxScanAgeMs = in->maxScanAgeMs;
  out->frequencyListLen = in->frequencyListLen;
  out->frequencyList.length = in->frequencyListLen * sizeof(uint32_t);
  CHPP_ASSERT((size_t)(*vlaOffset + out->frequencyList.length) <= payloadSize);
  if (out->frequencyList.length > 0 &&
      *vlaOffset + out->frequencyList.length <= payloadSize) {
    memcpy(&payload[*vlaOffset], in->frequencyList,
           in->frequencyListLen * sizeof(uint32_t));
    out->frequencyList.offset = *vlaOffset;
    *vlaOffset += out->frequencyList.length;
  } else {
    out->frequencyList.offset = 0;
  }
  out->ssidListLen = in->ssidListLen;

  struct ChppWifiSsidListItem *ssidList =
      (struct ChppWifiSsidListItem *)&payload[*vlaOffset];
  out->ssidList.length = in->ssidListLen * sizeof(struct ChppWifiSsidListItem);
  CHPP_ASSERT((size_t)(*vlaOffset + out->ssidList.length) <= payloadSize);
  if (out->ssidList.length > 0 &&
      *vlaOffset + out->ssidList.length <= payloadSize) {
    for (size_t i = 0; i < in->ssidListLen; i++) {
      chppWifiConvertSsidListItemFromChre(&in->ssidList[i], &ssidList[i]);
    }
    out->ssidList.offset = *vlaOffset;
    *vlaOffset += out->ssidList.length;
  } else {
    out->ssidList.offset = 0;
  }
  out->radioChainPref = in->radioChainPref;
}

// Encoding (CHRE --> CHPP) top-level functions

bool chppWifiScanEventFromChre(const struct chreWifiScanEvent *in,
                               struct ChppWifiScanEventWithHeader **out,
                               size_t *outSize) {
  CHPP_NOT_NULL(out);
  CHPP_NOT_NULL(outSize);

  size_t payloadSize = chppWifiSizeOfScanEventFromChre(in);
  *out = chppMalloc(payloadSize);
  if (*out != NULL) {
    uint8_t *payload = (uint8_t *)&(*out)->payload;
    uint16_t vlaOffset = sizeof(struct ChppWifiScanEvent);
    chppWifiConvertScanEventFromChre(in, &(*out)->payload, payload, payloadSize,
                                     &vlaOffset);
    *outSize = payloadSize;
    return true;
  }
  return false;
}

bool chppWifiScanParamsFromChre(const struct chreWifiScanParams *in,
                                struct ChppWifiScanParamsWithHeader **out,
                                size_t *outSize) {
  CHPP_NOT_NULL(out);
  CHPP_NOT_NULL(outSize);

  size_t payloadSize = chppWifiSizeOfScanParamsFromChre(in);
  *out = chppMalloc(payloadSize);
  if (*out != NULL) {
    uint8_t *payload = (uint8_t *)&(*out)->payload;
    uint16_t vlaOffset = sizeof(struct ChppWifiScanParams);
    chppWifiConvertScanParamsFromChre(in, &(*out)->payload, payload,
                                      payloadSize, &vlaOffset);
    *outSize = payloadSize;
    return true;
  }
  return false;
}

// Decoding (CHPP --> CHRE) conversion functions

static bool chppWifiConvertScanResultToChre(const struct ChppWifiScanResult *in,
                                            struct chreWifiScanResult *out) {
  out->ageMs = in->ageMs;
  out->capabilityInfo = in->capabilityInfo;
  out->ssidLen = in->ssidLen;
  memcpy(out->ssid, in->ssid, sizeof(out->ssid));
  memcpy(out->bssid, in->bssid, sizeof(out->bssid));
  out->flags = in->flags;
  out->rssi = in->rssi;
  out->band = in->band;
  out->primaryChannel = in->primaryChannel;
  out->centerFreqPrimary = in->centerFreqPrimary;
  out->centerFreqSecondary = in->centerFreqSecondary;
  out->channelWidth = in->channelWidth;
  out->securityMode = in->securityMode;
  out->radioChain = in->radioChain;
  out->rssiChain0 = in->rssiChain0;
  out->rssiChain1 = in->rssiChain1;
  memset(&out->reserved, 0, sizeof(out->reserved));

  return true;
}

static bool chppWifiConvertScanEventToChre(const struct ChppWifiScanEvent *in,
                                           struct chreWifiScanEvent *out,
                                           size_t inSize) {
  out->version = CHRE_WIFI_SCAN_EVENT_VERSION;
  out->resultCount = in->resultCount;
  out->resultTotal = in->resultTotal;
  out->eventIndex = in->eventIndex;
  out->scanType = in->scanType;
  out->ssidSetSize = in->ssidSetSize;
  out->scannedFreqListLen = in->scannedFreqListLen;
  out->referenceTime = in->referenceTime;

  if (in->scannedFreqList.length == 0) {
    out->scannedFreqList = NULL;
  } else {
    if (in->scannedFreqList.offset + in->scannedFreqList.length > inSize ||
        in->scannedFreqList.length !=
            in->scannedFreqListLen * sizeof(uint32_t)) {
      return false;
    }

    uint32_t *scannedFreqListOut =
        chppMalloc(in->scannedFreqListLen * sizeof(uint32_t));
    if (scannedFreqListOut == NULL) {
      return false;
    }

    memcpy(scannedFreqListOut,
           &((const uint8_t *)in)[in->scannedFreqList.offset],
           in->scannedFreqListLen * sizeof(uint32_t));
    out->scannedFreqList = scannedFreqListOut;
  }

  if (in->results.length == 0) {
    out->results = NULL;
  } else {
    if (in->results.offset + in->results.length > inSize ||
        in->results.length !=
            in->resultCount * sizeof(struct chreWifiScanResult)) {
      return false;
    }

    const struct ChppWifiScanResult *resultsIn =
        (const struct ChppWifiScanResult *)&(
            (const uint8_t *)in)[in->results.offset];

    struct chreWifiScanResult *resultsOut =
        chppMalloc(in->resultCount * sizeof(struct chreWifiScanResult));
    if (resultsOut == NULL) {
      return false;
    }

    for (size_t i = 0; i < in->resultCount; i++) {
      if (!chppWifiConvertScanResultToChre(&resultsIn[i], &resultsOut[i])) {
        return false;
      }
    }
    out->results = resultsOut;
  }

  out->radioChainPref = in->radioChainPref;

  return true;
}

static bool chppWifiConvertSsidListItemToChre(
    const struct ChppWifiSsidListItem *in, struct chreWifiSsidListItem *out) {
  out->ssidLen = in->ssidLen;
  memcpy(out->ssid, in->ssid, sizeof(out->ssid));

  return true;
}

static bool chppWifiConvertScanParamsToChre(const struct ChppWifiScanParams *in,
                                            struct chreWifiScanParams *out,
                                            size_t inSize) {
  out->scanType = in->scanType;
  out->maxScanAgeMs = in->maxScanAgeMs;
  out->frequencyListLen = in->frequencyListLen;

  if (in->frequencyList.length == 0) {
    out->frequencyList = NULL;
  } else {
    if (in->frequencyList.offset + in->frequencyList.length > inSize ||
        in->frequencyList.length != in->frequencyListLen * sizeof(uint32_t)) {
      return false;
    }

    uint32_t *frequencyListOut =
        chppMalloc(in->frequencyListLen * sizeof(uint32_t));
    if (frequencyListOut == NULL) {
      return false;
    }

    memcpy(frequencyListOut, &((const uint8_t *)in)[in->frequencyList.offset],
           in->frequencyListLen * sizeof(uint32_t));
    out->frequencyList = frequencyListOut;
  }

  out->ssidListLen = in->ssidListLen;

  if (in->ssidList.length == 0) {
    out->ssidList = NULL;
  } else {
    if (in->ssidList.offset + in->ssidList.length > inSize ||
        in->ssidList.length !=
            in->ssidListLen * sizeof(struct chreWifiSsidListItem)) {
      return false;
    }

    const struct ChppWifiSsidListItem *ssidListIn =
        (const struct ChppWifiSsidListItem *)&(
            (const uint8_t *)in)[in->ssidList.offset];

    struct chreWifiSsidListItem *ssidListOut =
        chppMalloc(in->ssidListLen * sizeof(struct chreWifiSsidListItem));
    if (ssidListOut == NULL) {
      return false;
    }

    for (size_t i = 0; i < in->ssidListLen; i++) {
      if (!chppWifiConvertSsidListItemToChre(&ssidListIn[i], &ssidListOut[i])) {
        return false;
      }
    }
    out->ssidList = ssidListOut;
  }

  out->radioChainPref = in->radioChainPref;

  return true;
}

// Decoding (CHPP --> CHRE) top-level functions

struct chreWifiScanEvent *chppWifiScanEventToChre(
    const struct ChppWifiScanEvent *in, size_t inSize) {
  struct chreWifiScanEvent *out = NULL;

  if (inSize >= sizeof(struct ChppWifiScanEvent)) {
    out = chppMalloc(sizeof(struct chreWifiScanEvent));
    if (out != NULL) {
      if (!chppWifiConvertScanEventToChre(in, out, inSize)) {
        CHPP_FREE_AND_NULLIFY(out);
      }
    }
  }

  return out;
}
struct chreWifiScanParams *chppWifiScanParamsToChre(
    const struct ChppWifiScanParams *in, size_t inSize) {
  struct chreWifiScanParams *out = NULL;

  if (inSize >= sizeof(struct ChppWifiScanParams)) {
    out = chppMalloc(sizeof(struct chreWifiScanParams));
    if (out != NULL) {
      if (!chppWifiConvertScanParamsToChre(in, out, inSize)) {
        CHPP_FREE_AND_NULLIFY(out);
      }
    }
  }

  return out;
}
