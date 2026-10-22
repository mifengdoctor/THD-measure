#ifndef ADC_CAPTURE_H
#define ADC_CAPTURE_H

#include <stdbool.h>
#include <stdint.h>

void ADC_Capture_Init(void);

/*
 * 启动一次 ADC + DMA 采集。
 */
bool ADC_Capture_Start(void);

/*
 * 得到 ADC 采样缓冲区。
 *
 * 返回的是 uint16_t 类型的 12 位 ADC 原始值。
 */
const uint16_t *ADC_Capture_GetSamples(void);

/*
 * 本轮 DMA 采集是否完成。
 */
bool ADC_Capture_IsDone(void);

#endif