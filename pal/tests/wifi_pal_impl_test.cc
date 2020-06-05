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

#include "chre/pal/wifi.h"
#include "chre/platform/condition_variable.h"
#include "chre/platform/log.h"
#include "chre/platform/mutex.h"
#include "chre/platform/shared/pal_system_api.h"
#include "chre/platform/system_time.h"
#include "chre/util/dynamic_vector.h"
#include "chre/util/lock_guard.h"
#include "chre/util/nanoapp/wifi.h"
#include "chre/util/time.h"
#include "gtest/gtest.h"

#include <cinttypes>

namespace {

using ::chre::Nanoseconds;
using ::chre::Seconds;
using ::chre::SystemTime;

// TODO: Move these as a part of the test fixture
uint8_t gErrorCode = CHRE_ERROR_LAST;
uint32_t gNumScanResultCount = 0;
bool gLastScanEventReceived = false;

//! A list to store the scan results
chre::DynamicVector<chreWifiScanEvent *> gScanEventList;

//! Mutex to protect global variables
chre::Mutex gMutex;

chre::ConditionVariable gCondVar;

void logChreWifiResult(const chreWifiScanResult &result) {
  const char *ssidStr = "<non-printable>";
  char ssidBuffer[chre::kMaxSsidStrLen];
  if (result.ssidLen == 0) {
    ssidStr = "<empty>";
  } else if (chre::parseSsidToStr(ssidBuffer, sizeof(ssidBuffer), result.ssid,
                                  result.ssidLen)) {
    ssidStr = ssidBuffer;
  }

  LOGI("Found network with SSID: %s", ssidStr);
  const char *bssidStr = "<non-printable>";
  char bssidBuffer[chre::kBssidStrLen];
  if (chre::parseBssidToStr(result.bssid, bssidBuffer, sizeof(bssidBuffer))) {
    bssidStr = bssidBuffer;
  }

  LOGI("  age (ms): %" PRIu32, result.ageMs);
  LOGI("  capability info: 0x%" PRIx16, result.capabilityInfo);
  LOGI("  bssid: %s", bssidStr);
  LOGI("  flags: 0x%" PRIx8, result.flags);
  LOGI("  rssi: %" PRId8 "dBm", result.rssi);
  LOGI("  band: %s (%" PRIu8 ")", chre::parseChreWifiBand(result.band),
       result.band);
  LOGI("  primary channel: %" PRIu32, result.primaryChannel);
  LOGI("  center frequency primary: %" PRIu32, result.centerFreqPrimary);
  LOGI("  center frequency secondary: %" PRIu32, result.centerFreqSecondary);
  LOGI("  channel width: %" PRIu8, result.channelWidth);
  LOGI("  security mode: 0x%" PRIx8, result.securityMode);
}

void chrePalScanMonitorStatusChangeCallback(bool enabled, uint8_t errorCode) {
  // TODO:
}

void chrePalScanResponseCallback(bool pending, uint8_t errorCode) {
  LOGI("Received scan response with pending %d error %" PRIu8, pending,
       errorCode);
  chre::LockGuard<chre::Mutex> lock(gMutex);
  gErrorCode = errorCode;
  gCondVar.notify_one();
}

void chrePalScanEventCallback(struct chreWifiScanEvent *event) {
  if (event == nullptr) {
    LOGE("Got null scan event");
  } else {
    {
      chre::LockGuard<chre::Mutex> lock(gMutex);
      gScanEventList.push_back(event);
      gNumScanResultCount += event->resultCount;
      gLastScanEventReceived = (gNumScanResultCount == event->resultTotal);
    }

    gCondVar.notify_one();
  }
}

void chrePalRangingEventCallback(uint8_t errorCode,
                                 struct chreWifiRangingEvent *event) {
  // TODO:
}

class PalWifiTest : public ::testing::Test {
 protected:
  void SetUp() override {
    api_ = chrePalWifiGetApi(CHRE_PAL_WIFI_API_CURRENT_VERSION);
    ASSERT_NE(api_, nullptr);
    EXPECT_EQ(api_->moduleVersion, CHRE_PAL_WIFI_API_CURRENT_VERSION);

    // Open the PAL API
    static const struct chrePalWifiCallbacks kCallbacks = {
        .scanMonitorStatusChangeCallback =
            chrePalScanMonitorStatusChangeCallback,
        .scanResponseCallback = chrePalScanResponseCallback,
        .scanEventCallback = chrePalScanEventCallback,
        .rangingEventCallback = chrePalRangingEventCallback,
    };
    ASSERT_TRUE(api_->open(&chre::gChrePalSystemApi, &kCallbacks));
  }

  void TearDown() override {
    api_->close();
  }

  const struct chrePalWifiApi *api_;
};

}  // anonymous namespace

TEST_F(PalWifiTest, ScanAsyncTest) {
  // Request a WiFi scan
  chre::LockGuard<chre::Mutex> lock(gMutex);
  gNumScanResultCount = 0;
  gLastScanEventReceived = false;

  struct chreWifiScanParams params = {};
  params.scanType = CHRE_WIFI_SCAN_TYPE_ACTIVE;
  params.maxScanAgeMs = 5000;  // 5 seconds
  params.frequencyListLen = 0;
  params.ssidListLen = 0;
  params.radioChainPref = CHRE_WIFI_RADIO_CHAIN_PREF_DEFAULT;
  ASSERT_TRUE(api_->requestScan(&params));

  const Nanoseconds kTimeoutNs = Nanoseconds(CHRE_WIFI_SCAN_RESULT_TIMEOUT_NS);
  Nanoseconds end = SystemTime::getMonotonicTime() + kTimeoutNs;
  gErrorCode = CHRE_ERROR_LAST;
  while (gErrorCode == CHRE_ERROR_LAST &&
         SystemTime::getMonotonicTime() < end) {
    gCondVar.wait_for(gMutex, kTimeoutNs);
  }
  ASSERT_LT(SystemTime::getMonotonicTime(), end);
  ASSERT_EQ(gErrorCode, CHRE_ERROR_NONE);

  // The CHRE API only poses timeout requirements on the async response. Use
  // the same timeout to receive the scan results to avoid blocking forever.
  end = SystemTime::getMonotonicTime() + kTimeoutNs;
  while (!gLastScanEventReceived && SystemTime::getMonotonicTime() < end) {
    gCondVar.wait_for(gMutex, kTimeoutNs);
  }

  for (auto *event : gScanEventList) {
    // TODO: Sanity check values
    for (uint8_t i = 0; i < event->resultCount; i++) {
      const chreWifiScanResult &result = event->results[i];
      logChreWifiResult(result);
    }
    api_->releaseScanEvent(event);
  }

  EXPECT_TRUE(gLastScanEventReceived);
  EXPECT_GT(gNumScanResultCount, 0);
}