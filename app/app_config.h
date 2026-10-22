#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * ADC 一次采集的样本数量。
 *
 * ADC 开启 Repeat Mode 后连续采样：
 * 8192 / 4044000 ≈ 2.026 ms
 */
#define ADC_CAPTURE_SAMPLES          (8192U)

/*
 * 当前 ADC 配置为 Repeat Mode，
 * TIMG0 触发后 ADC 以最大速度连续转换。
 * 根据 1 kHz 输入实测，校准后的等效采样率约为 4.044 MHz。
 */
#define ADC_SAMPLE_RATE_HZ           (4044000.0f)

#define HARMONIC_COUNT               (5U)
#define WAVE_POINT_COUNT             (195U)
#define MEASURE_PERIOD_MS            (500U)

#endif