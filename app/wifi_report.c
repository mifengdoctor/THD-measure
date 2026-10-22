#include "wifi_report.h"

#include "ti_msp_dl_config.h"

static uint32_t g_wifi_sequence = 0U;

static void wifi_send_byte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(UART_1_INST, data);
}

static void wifi_send_string(const char *text)
{
    while (*text != '\0') {
        wifi_send_byte((uint8_t)*text);
        text++;
    }
}

static void wifi_send_uint(uint32_t value)
{
    char digits[10];
    uint8_t count = 0U;
    uint8_t i;

    if (value == 0U) {
        wifi_send_byte((uint8_t)'0');
        return;
    }

    while ((value > 0U) && (count < 10U)) {
        digits[count] =
            (char)('0' + (value % 10U));

        value /= 10U;
        count++;
    }

    for (i = count; i > 0U; i--) {
        wifi_send_byte((uint8_t)digits[i - 1U]);
    }
}

static void wifi_send_scaled(
    uint32_t value,
    uint8_t decimal_count)
{
    uint32_t divisor = 1U;
    uint8_t i;

    for (i = 0U; i < decimal_count; i++) {
        divisor *= 10U;
    }

    wifi_send_uint(value / divisor);

    if (decimal_count > 0U) {
        wifi_send_byte((uint8_t)'.');

        divisor /= 10U;

        while (divisor > 0U) {
            wifi_send_byte(
                (uint8_t)(
                    '0' +
                    ((value / divisor) % 10U)));

            divisor /= 10U;
        }
    }
}

static uint32_t wifi_float_to_scaled(
    float value,
    uint32_t scale)
{
    if (value < 0.0f) {
        value = 0.0f;
    }

    if (value > 10000.0f) {
        value = 10000.0f;
    }

    return (uint32_t)(
        value * (float)scale + 0.5f);
}

void WiFi_Report_Init(void)
{
    g_wifi_sequence = 0U;
}

void WiFi_Report_Measurement(
    const MeasurementResult *result,
    const uint8_t *wave_points,
    uint16_t point_count)
{
    uint16_t i;

    if ((result == 0) ||
        (!result->valid)) {

        return;
    }

    g_wifi_sequence++;

    /*
     * 手机 App 支持下面的 JSON：
     * {"seq":1,"freq":1000,"thd":8.35,
     *  "h":[1.000,0.024,...],
     *  "wave":[128,140,...]}
     */
    wifi_send_string("{\"seq\":");
    wifi_send_uint(g_wifi_sequence);

    wifi_send_string(",\"freq\":");
    wifi_send_uint(
        (uint32_t)(result->frequency_hz + 0.5f));

    wifi_send_string(",\"thd\":");
    wifi_send_scaled(
        wifi_float_to_scaled(
            result->thd_percent,
            100U),
        2U);

    wifi_send_string(",\"h\":[");

    for (i = 0U; i < HARMONIC_COUNT; i++) {
        wifi_send_scaled(
            wifi_float_to_scaled(
                result->harmonic[i],
                1000U),
            3U);

        if (i < (HARMONIC_COUNT - 1U)) {
            wifi_send_byte((uint8_t)',');
        }
    }

    wifi_send_string("],\"wave\":[");

    if ((wave_points != 0) &&
        (point_count > 0U)) {

        for (i = 0U; i < point_count; i++) {
            wifi_send_uint((uint32_t)wave_points[i]);

            if (i < (point_count - 1U)) {
                wifi_send_byte((uint8_t)',');
            }
        }
    }

    wifi_send_string("]}\n");
}