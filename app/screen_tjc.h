#ifndef SCREEN_TJC_H
#define SCREEN_TJC_H

#include <stdint.h>

#include "measure_thd.h"

void Screen_TJC_Init(void);

void Screen_TJC_SetStatus(const char *status);

void Screen_TJC_SendNoSignal(void);

void Screen_TJC_SendAdcDebug(
    uint16_t min_value,
    uint16_t max_value);

void Screen_TJC_SendMeasurement(
    const MeasurementResult *result,
    const uint8_t *wave_points,
    uint16_t point_count);

#endif