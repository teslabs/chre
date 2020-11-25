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
// Date: 2020-12-01 07:00:20 UTC
// Source: chre_api/include/chre_api/chre/gnss.h @ commit 5e7b258d

// DO NOT modify this file directly, as those changes will be lost the next
// time the script is executed

#include "chpp/common/gnss_types.h"
#include "chpp/macros.h"
#include "chpp/memory.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Encoding (CHRE --> CHPP) size functions

//! @return number of bytes required to represent the given
//! chreGnssDataEvent along with the CHPP header as
//! struct ChppGnssDataEventWithHeader
static size_t chppGnssSizeOfDataEventFromChre(
    const struct chreGnssDataEvent *dataEvent) {
  size_t encodedSize = sizeof(struct ChppGnssDataEventWithHeader);
  encodedSize +=
      dataEvent->measurement_count * sizeof(struct ChppGnssMeasurement);
  return encodedSize;
}

// Encoding (CHRE --> CHPP) conversion functions

static void chppGnssConvertClockFromChre(const struct chreGnssClock *in,
                                         struct ChppGnssClock *out) {
  out->time_ns = in->time_ns;
  out->full_bias_ns = in->full_bias_ns;
  out->bias_ns = in->bias_ns;
  out->drift_nsps = in->drift_nsps;
  out->bias_uncertainty_ns = in->bias_uncertainty_ns;
  out->drift_uncertainty_nsps = in->drift_uncertainty_nsps;
  out->hw_clock_discontinuity_count = in->hw_clock_discontinuity_count;
  out->flags = in->flags;
  memset(&out->reserved, 0, sizeof(out->reserved));
}

static void chppGnssConvertMeasurementFromChre(
    const struct chreGnssMeasurement *in, struct ChppGnssMeasurement *out) {
  out->time_offset_ns = in->time_offset_ns;
  out->accumulated_delta_range_um = in->accumulated_delta_range_um;
  out->received_sv_time_in_ns = in->received_sv_time_in_ns;
  out->received_sv_time_uncertainty_in_ns =
      in->received_sv_time_uncertainty_in_ns;
  out->pseudorange_rate_mps = in->pseudorange_rate_mps;
  out->pseudorange_rate_uncertainty_mps = in->pseudorange_rate_uncertainty_mps;
  out->accumulated_delta_range_uncertainty_m =
      in->accumulated_delta_range_uncertainty_m;
  out->c_n0_dbhz = in->c_n0_dbhz;
  out->snr_db = in->snr_db;
  out->state = in->state;
  out->accumulated_delta_range_state = in->accumulated_delta_range_state;
  out->svid = in->svid;
  out->constellation = in->constellation;
  out->multipath_indicator = in->multipath_indicator;
  out->carrier_frequency_hz = in->carrier_frequency_hz;
}

static void chppGnssConvertDataEventFromChre(const struct chreGnssDataEvent *in,
                                             struct ChppGnssDataEvent *out,
                                             uint8_t *payload,
                                             size_t payloadSize,
                                             uint16_t *vlaOffset) {
  out->version = CHRE_GNSS_DATA_EVENT_VERSION;
  out->measurement_count = in->measurement_count;
  memset(&out->reserved, 0, sizeof(out->reserved));
  chppGnssConvertClockFromChre(&in->clock, &out->clock);

  struct ChppGnssMeasurement *measurements =
      (struct ChppGnssMeasurement *)&payload[*vlaOffset];
  out->measurements.length =
      in->measurement_count * sizeof(struct ChppGnssMeasurement);
  CHPP_ASSERT((size_t)(*vlaOffset + out->measurements.length) <= payloadSize);
  if (out->measurements.length > 0 &&
      *vlaOffset + out->measurements.length <= payloadSize) {
    for (size_t i = 0; i < in->measurement_count; i++) {
      chppGnssConvertMeasurementFromChre(&in->measurements[i],
                                         &measurements[i]);
    }
    out->measurements.offset = *vlaOffset;
    *vlaOffset += out->measurements.length;
  } else {
    out->measurements.offset = 0;
  }
}

static void chppGnssConvertLocationEventFromChre(
    const struct chreGnssLocationEvent *in, struct ChppGnssLocationEvent *out) {
  out->timestamp = in->timestamp;
  out->latitude_deg_e7 = in->latitude_deg_e7;
  out->longitude_deg_e7 = in->longitude_deg_e7;
  out->altitude = in->altitude;
  out->speed = in->speed;
  out->bearing = in->bearing;
  out->accuracy = in->accuracy;
  out->flags = in->flags;
  memset(&out->reserved, 0, sizeof(out->reserved));
  out->altitude_accuracy = in->altitude_accuracy;
  out->speed_accuracy = in->speed_accuracy;
  out->bearing_accuracy = in->bearing_accuracy;
}

// Encoding (CHRE --> CHPP) top-level functions

bool chppGnssDataEventFromChre(const struct chreGnssDataEvent *in,
                               struct ChppGnssDataEventWithHeader **out,
                               size_t *outSize) {
  CHPP_NOT_NULL(out);
  CHPP_NOT_NULL(outSize);

  size_t payloadSize = chppGnssSizeOfDataEventFromChre(in);
  *out = chppMalloc(payloadSize);
  if (*out != NULL) {
    uint8_t *payload = (uint8_t *)&(*out)->payload;
    uint16_t vlaOffset = sizeof(struct ChppGnssDataEvent);
    chppGnssConvertDataEventFromChre(in, &(*out)->payload, payload, payloadSize,
                                     &vlaOffset);
    *outSize = payloadSize;
    return true;
  }
  return false;
}

bool chppGnssLocationEventFromChre(const struct chreGnssLocationEvent *in,
                                   struct ChppGnssLocationEventWithHeader **out,
                                   size_t *outSize) {
  CHPP_NOT_NULL(out);
  CHPP_NOT_NULL(outSize);

  size_t payloadSize = sizeof(struct ChppGnssLocationEventWithHeader);
  *out = chppMalloc(payloadSize);
  if (*out != NULL) {
    chppGnssConvertLocationEventFromChre(in, &(*out)->payload);
    *outSize = payloadSize;
    return true;
  }
  return false;
}

// Decoding (CHPP --> CHRE) conversion functions

static bool chppGnssConvertClockToChre(const struct ChppGnssClock *in,
                                       struct chreGnssClock *out) {
  out->time_ns = in->time_ns;
  out->full_bias_ns = in->full_bias_ns;
  out->bias_ns = in->bias_ns;
  out->drift_nsps = in->drift_nsps;
  out->bias_uncertainty_ns = in->bias_uncertainty_ns;
  out->drift_uncertainty_nsps = in->drift_uncertainty_nsps;
  out->hw_clock_discontinuity_count = in->hw_clock_discontinuity_count;
  out->flags = in->flags;
  memset(&out->reserved, 0, sizeof(out->reserved));

  return true;
}

static bool chppGnssConvertMeasurementToChre(
    const struct ChppGnssMeasurement *in, struct chreGnssMeasurement *out) {
  out->time_offset_ns = in->time_offset_ns;
  out->accumulated_delta_range_um = in->accumulated_delta_range_um;
  out->received_sv_time_in_ns = in->received_sv_time_in_ns;
  out->received_sv_time_uncertainty_in_ns =
      in->received_sv_time_uncertainty_in_ns;
  out->pseudorange_rate_mps = in->pseudorange_rate_mps;
  out->pseudorange_rate_uncertainty_mps = in->pseudorange_rate_uncertainty_mps;
  out->accumulated_delta_range_uncertainty_m =
      in->accumulated_delta_range_uncertainty_m;
  out->c_n0_dbhz = in->c_n0_dbhz;
  out->snr_db = in->snr_db;
  out->state = in->state;
  out->accumulated_delta_range_state = in->accumulated_delta_range_state;
  out->svid = in->svid;
  out->constellation = in->constellation;
  out->multipath_indicator = in->multipath_indicator;
  out->carrier_frequency_hz = in->carrier_frequency_hz;

  return true;
}

static bool chppGnssConvertDataEventToChre(const struct ChppGnssDataEvent *in,
                                           struct chreGnssDataEvent *out,
                                           size_t inSize) {
  out->version = CHRE_GNSS_DATA_EVENT_VERSION;
  out->measurement_count = in->measurement_count;
  memset(&out->reserved, 0, sizeof(out->reserved));
  if (!chppGnssConvertClockToChre(&in->clock, &out->clock)) {
    return false;
  }

  if (in->measurements.length == 0) {
    out->measurements = NULL;
  } else {
    if (in->measurements.offset + in->measurements.length > inSize ||
        in->measurements.length !=
            in->measurement_count * sizeof(struct chreGnssMeasurement)) {
      return false;
    }

    const struct ChppGnssMeasurement *measurementsIn =
        (const struct ChppGnssMeasurement *)&(
            (const uint8_t *)in)[in->measurements.offset];

    struct chreGnssMeasurement *measurementsOut =
        chppMalloc(in->measurement_count * sizeof(struct chreGnssMeasurement));
    if (measurementsOut == NULL) {
      return false;
    }

    for (size_t i = 0; i < in->measurement_count; i++) {
      if (!chppGnssConvertMeasurementToChre(&measurementsIn[i],
                                            &measurementsOut[i])) {
        return false;
      }
    }
    out->measurements = measurementsOut;
  }

  return true;
}

static bool chppGnssConvertLocationEventToChre(
    const struct ChppGnssLocationEvent *in, struct chreGnssLocationEvent *out) {
  out->timestamp = in->timestamp;
  out->latitude_deg_e7 = in->latitude_deg_e7;
  out->longitude_deg_e7 = in->longitude_deg_e7;
  out->altitude = in->altitude;
  out->speed = in->speed;
  out->bearing = in->bearing;
  out->accuracy = in->accuracy;
  out->flags = in->flags;
  memset(&out->reserved, 0, sizeof(out->reserved));
  out->altitude_accuracy = in->altitude_accuracy;
  out->speed_accuracy = in->speed_accuracy;
  out->bearing_accuracy = in->bearing_accuracy;

  return true;
}

// Decoding (CHPP --> CHRE) top-level functions

struct chreGnssDataEvent *chppGnssDataEventToChre(
    const struct ChppGnssDataEvent *in, size_t inSize) {
  struct chreGnssDataEvent *out = NULL;

  if (inSize >= sizeof(struct ChppGnssDataEvent)) {
    out = chppMalloc(sizeof(struct chreGnssDataEvent));
    if (out != NULL) {
      if (!chppGnssConvertDataEventToChre(in, out, inSize)) {
        CHPP_FREE_AND_NULLIFY(out);
      }
    }
  }

  return out;
}
struct chreGnssLocationEvent *chppGnssLocationEventToChre(
    const struct ChppGnssLocationEvent *in, size_t inSize) {
  struct chreGnssLocationEvent *out = NULL;

  if (inSize >= sizeof(struct ChppGnssLocationEvent)) {
    out = chppMalloc(sizeof(struct chreGnssLocationEvent));
    if (out != NULL) {
      if (!chppGnssConvertLocationEventToChre(in, out)) {
        CHPP_FREE_AND_NULLIFY(out);
      }
    }
  }

  return out;
}
