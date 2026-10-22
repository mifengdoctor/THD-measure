#ifndef MEASURE_THD_H
#define MEASURE_THD_H

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"

typedef struct
{
    bool valid;

    float frequency_hz;
    float period_samples;

    float dc_value;
    float peak_value;

    /*
     * 总谐波失真，单位是百分比。
     * 例如 12.34 表示 12.34%。
     */
    float thd_percent;

    /*
     * harmonic[0] = 基波，固定为 1.000
     * harmonic[1] = 2 次谐波 / 基波
     * harmonic[2] = 3 次谐波 / 基波
     * harmonic[3] = 4 次谐波 / 基波
     * harmonic[4] = 5 次谐波 / 基波
     */
    float harmonic[HARMONIC_COUNT];

    uint16_t first_rising_edge;
} MeasurementResult;

bool Measure_THD_Process(
    const uint16_t *samples,
    uint16_t sample_count,
    MeasurementResult *result);

uint16_t Measure_THD_BuildWaveform(
    const uint16_t *samples,
    uint16_t sample_count,
    const MeasurementResult *result,
    uint8_t *points,
    uint16_t max_points);

#endif