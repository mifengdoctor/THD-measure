#ifndef WIFI_REPORT_H
#define WIFI_REPORT_H

#include <stdint.h>

#include "measure_thd.h"

void WiFi_Report_Init(void);

void WiFi_Report_Measurement(
    const MeasurementResult *result,
    const uint8_t *wave_points,
    uint16_t point_count);

#endif