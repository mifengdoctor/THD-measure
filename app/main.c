/*
 * MSPM0G3507
 * 2021 年全国大学生电子设计竞赛 A 题
 * 信号失真度测量装置
 */

#include "adc_capture.h"
#include "app_config.h"
#include "bsp_uart_tjc.h"
#include "measure_thd.h"
#include "screen_tjc.h"
#include "ti_msp_dl_config.h"
#include "wifi_report.h"

#define ADC_CAPTURE_TIMEOUT_MS    (50U)

static void app_delay_ms(uint32_t milliseconds)
{
    while (milliseconds > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
        milliseconds--;
    }
}

static bool app_measure_once(
    MeasurementResult *measurement,
    uint8_t *wave_points,
    uint16_t *wave_count)
{
    const uint16_t *samples;
    uint32_t timeout_ms;
    uint16_t min_value = 4095U;
    uint16_t max_value = 0U;
    uint16_t i;

    *wave_count = 0U;

    if (!ADC_Capture_Start()) {
        Screen_TJC_SetStatus("CAP START FAIL");
        return false;
    }

    timeout_ms = ADC_CAPTURE_TIMEOUT_MS;

    while ((!ADC_Capture_IsDone()) &&
           (timeout_ms > 0U)) {
        app_delay_ms(1U);
        timeout_ms--;
    }

    if (!ADC_Capture_IsDone()) {
        Screen_TJC_SetStatus("CAP TIMEOUT");
        return false;
    }

    samples = ADC_Capture_GetSamples();

    for (i = 0U; i < ADC_CAPTURE_SAMPLES; i++) {
        uint16_t value = samples[i];

        if (value < min_value) {
            min_value = value;
        }

        if (value > max_value) {
            max_value = value;
        }
    }

    if (!Measure_THD_Process(
            samples,
            ADC_CAPTURE_SAMPLES,
            measurement)) {

        Screen_TJC_SendAdcDebug(min_value, max_value);
        return false;
    }

    *wave_count = Measure_THD_BuildWaveform(
        samples,
        ADC_CAPTURE_SAMPLES,
        measurement,
        wave_points,
        WAVE_POINT_COUNT);

    Screen_TJC_SendMeasurement(
        measurement,
        wave_points,
        *wave_count);

    /*
     * 同一份测量结果同时通过 UART1 发给 ESP8266。
     */
    WiFi_Report_Measurement(
        measurement,
        wave_points,
        *wave_count);

    return true;
}

int main(void)
{
    MeasurementResult measurement;

    uint8_t wave_points[WAVE_POINT_COUNT];
    uint16_t wave_count;

    bool measurement_ok;

    SYSCFG_DL_init();

    BSP_UART_TJC_Init();
    WiFi_Report_Init();
    ADC_Capture_Init();

    app_delay_ms(500U);

    Screen_TJC_Init();

    while (1) {
        if (BSP_UART_TJC_ReadStartCommand()) {

            Screen_TJC_SetStatus("MEASURING");

            measurement_ok = app_measure_once(
                &measurement,
                wave_points,
                &wave_count);

            if (measurement_ok) {
                Screen_TJC_SetStatus("DONE");
            }

            DL_GPIO_togglePins(
                GPIO_GRP_0_PORT,
                GPIO_GRP_0_PIN_0_PIN);
        }

        app_delay_ms(1U);
    }
}