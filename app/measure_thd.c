#include "measure_thd.h"

#include <math.h>
#include <string.h>

#include "app_config.h"

#define PI_F    (3.14159265358979323846f)

/* ---------- 临时算法诊断变量 ---------- */
volatile uint8_t g_thd_debug_stage = 0U;
volatile uint16_t g_thd_debug_edge_count = 0U;
volatile float g_thd_debug_period_samples = 0.0f;
volatile float g_thd_debug_frequency_hz = 0.0f;
volatile float g_thd_debug_amplitude0 = 0.0f;
/* -------------------------------------- */

static float measure_mean(
    const uint16_t *samples,
    uint16_t sample_count)
{
    uint32_t sum = 0U;
    uint16_t i;

    for (i = 0U; i < sample_count; i++) {
        sum += samples[i];
    }

    return (float)sum / (float)sample_count;
}

static float measure_interpolate(
    const uint16_t *samples,
    uint16_t sample_count,
    float position)
{
    uint16_t index;
    float fraction;

    if (position <= 0.0f) {
        return (float)samples[0];
    }

    if (position >= (float)(sample_count - 1U)) {
        return (float)samples[sample_count - 1U];
    }

    index = (uint16_t)position;
    fraction = position - (float)index;

    return (float)samples[index] +
           fraction * ((float)samples[index + 1U] -
                       (float)samples[index]);
}

bool Measure_THD_Process(
    const uint16_t *samples,
    uint16_t sample_count,
    MeasurementResult *result)
{
    uint16_t i;
    uint16_t min_value = 4095U;
    uint16_t max_value = 0U;

    float mean;
    float hysteresis;

    float first_edge = 0.0f;
    float last_edge = 0.0f;
    float edge_span = 0.0f;
    uint16_t edge_count = 0U;
    bool armed_for_rising = false;

    float period_samples;
    float frequency_hz;
    float resample_span;
    uint16_t cycle_count;
    float phase_step;

    float amplitude[HARMONIC_COUNT];
    float harmonic_power;
    uint8_t k;

    g_thd_debug_stage = 0U;
    g_thd_debug_edge_count = 0U;
    g_thd_debug_period_samples = 0.0f;
    g_thd_debug_frequency_hz = 0.0f;
    g_thd_debug_amplitude0 = 0.0f;

    if ((samples == 0) ||
        (result == 0) ||
        (sample_count < 256U)) {

        g_thd_debug_stage = 9U;
        return false;
    }

    memset(result, 0, sizeof(*result));

    for (i = 0U; i < sample_count; i++) {
        uint16_t value = samples[i];

        if (value < min_value) {
            min_value = value;
        }

        if (value > max_value) {
            max_value = value;
        }
    }

    if ((uint16_t)(max_value - min_value) < 16U) {
        g_thd_debug_stage = 6U;
        return false;
    }

    mean = measure_mean(samples, sample_count);

    /*
     * 使用峰峰值的 1/8 作为上升沿迟滞，
     * 减少噪声在平均值附近造成重复过零。
     */
    hysteresis =
        ((float)max_value - (float)min_value) / 8.0f;

    if (hysteresis < 2.0f) {
        hysteresis = 2.0f;
    }

    for (i = 1U; i < sample_count; i++) {
        float previous = (float)samples[i - 1U] - mean;
        float current = (float)samples[i] - mean;

        if (!armed_for_rising) {
            if (current <= -hysteresis) {
                armed_for_rising = true;
            }
        } else if (current >= hysteresis) {
            float denominator = current - previous;
            float fraction;
            float position;

            if (denominator <= 0.0f) {
                armed_for_rising = false;
                continue;
            }

            fraction =
                (hysteresis - previous) / denominator;

            if (fraction < 0.0f) {
                fraction = 0.0f;
            } else if (fraction > 1.0f) {
                fraction = 1.0f;
            }

            position =
                (float)(i - 1U) + fraction;

            if (edge_count == 0U) {
                first_edge = position;
            } else {
                edge_span += position - last_edge;
            }

            last_edge = position;
            edge_count++;
            armed_for_rising = false;
        }
    }

    g_thd_debug_edge_count = edge_count;

    if (edge_count < 2U) {
        g_thd_debug_stage = 1U;
        return false;
    }

    period_samples =
        edge_span / (float)(edge_count - 1U);

    g_thd_debug_period_samples = period_samples;

    if ((period_samples < 2.0f) ||
        (period_samples > (float)sample_count)) {

        g_thd_debug_stage = 2U;
        return false;
    }

    frequency_hz =
        ADC_SAMPLE_RATE_HZ / period_samples;

    g_thd_debug_frequency_hz = frequency_hz;

    if ((frequency_hz < 800.0f) ||
        (frequency_hz > 120000.0f)) {

        g_thd_debug_stage = 3U;
        return false;
    }

    /*
     * 用整数个周期进行相干重采样，
     * 再做 1~5 次谐波的 Goertzel 等效 DFT。
     */
    cycle_count = (uint16_t)(
        (float)sample_count / period_samples);

    if (cycle_count == 0U) {
        g_thd_debug_stage = 4U;
        return false;
    }

    resample_span =
        period_samples * (float)cycle_count;

    phase_step =
        (2.0f * PI_F * (float)cycle_count) /
        (float)sample_count;

    for (k = 0U; k < HARMONIC_COUNT; k++) {
        float omega =
            phase_step * (float)(k + 1U);

        float coefficient_c = cosf(omega);
        float coefficient_s = sinf(omega);

        float oscillator_c = 1.0f;
        float oscillator_s = 0.0f;

        float real = 0.0f;
        float imaginary = 0.0f;

        for (i = 0U; i < sample_count; i++) {
            float source_position =
                resample_span * (float)i /
                (float)sample_count;

            float value =
                measure_interpolate(
                    samples,
                    sample_count,
                    source_position) - mean;

            float next_c;
            float next_s;

            real += value * oscillator_c;
            imaginary -= value * oscillator_s;

            next_c =
                oscillator_c * coefficient_c -
                oscillator_s * coefficient_s;

            next_s =
                oscillator_s * coefficient_c +
                oscillator_c * coefficient_s;

            oscillator_c = next_c;
            oscillator_s = next_s;
        }

        amplitude[k] =
            (2.0f / (float)sample_count) *
            sqrtf((real * real) + (imaginary * imaginary));
    }

    g_thd_debug_amplitude0 = amplitude[0];

    if (amplitude[0] < 4.0f) {
        g_thd_debug_stage = 5U;
        return false;
    }

    result->valid = true;
    result->frequency_hz = frequency_hz;
    result->period_samples = period_samples;
    result->dc_value = mean;

    result->peak_value =
        ((float)max_value - (float)min_value) / 2.0f;

    result->first_rising_edge =
        (uint16_t)(first_edge + 0.5f);

    result->harmonic[0] = 1.0f;

    harmonic_power = 0.0f;

    for (k = 1U; k < HARMONIC_COUNT; k++) {
        result->harmonic[k] =
            amplitude[k] / amplitude[0];

        harmonic_power +=
            amplitude[k] * amplitude[k];
    }

    result->thd_percent =
        (100.0f * sqrtf(harmonic_power)) / amplitude[0];

    return true;
}

uint16_t Measure_THD_BuildWaveform(
    const uint16_t *samples,
    uint16_t sample_count,
    const MeasurementResult *result,
    uint8_t *points,
    uint16_t max_points)
{
    uint16_t point_count;
    uint16_t i;

    float start_position;
    float max_start_position;

    if ((samples == 0) ||
        (result == 0) ||
        (points == 0) ||
        (max_points == 0U)) {

        return 0U;
    }

    point_count =
        (max_points < WAVE_POINT_COUNT) ?
        max_points : WAVE_POINT_COUNT;

    if (point_count < 2U) {
        return 0U;
    }

    if (!result->valid) {
        memset(points, 128, point_count);
        return point_count;
    }

    /*
     * 一个完整周期必须完全落在采样缓冲区内部。
     *
     * 如果 first_rising_edge 靠近缓冲区末尾，
     * 原来直接从它开始取一个周期，就会越过缓冲区，
     * 插值函数只能一直返回最后一个采样点，
     * 最终表现为“半个周期 + 一条直线”。
     */
    max_start_position =
        (float)(sample_count - 1U) -
        result->period_samples;

    if (max_start_position < 0.0f) {
        return 0U;
    }

    start_position =
        (float)result->first_rising_edge;

    if (start_position > max_start_position) {
        start_position = max_start_position;
    }

    for (i = 0U; i < point_count; i++) {
        float offset;
        float position;
        float value;
        float centered;
        float scaled;

        offset =
            result->period_samples * (float)i /
            (float)(point_count - 1U);

        position =
            start_position + offset;

        value = measure_interpolate(
            samples,
            sample_count,
            position);

        centered = value - result->dc_value;

        scaled =
            128.0f +
            100.0f * centered / result->peak_value;

        if (scaled < 0.0f) {
            scaled = 0.0f;
        } else if (scaled > 255.0f) {
            scaled = 255.0f;
        }

        points[i] =
            (uint8_t)(scaled + 0.5f);
    }

    return point_count;
}