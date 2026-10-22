#ifndef BSP_UART_TJC_H
#define BSP_UART_TJC_H

#include <stdbool.h>
#include <stdint.h>

void BSP_UART_TJC_Init(void);

void BSP_UART_TJC_SendByte(uint8_t data);

void BSP_UART_TJC_SendString(const char *text);

void BSP_UART_TJC_SendBytes(const uint8_t *data, uint16_t length);

bool BSP_UART_TJC_ReadByte(uint8_t *data);

/*
 * 读取串口屏发送来的“开始测量”按钮命令。
 *
 * 命令帧：
 * 55 10 00 00 FF FF FF
 */
bool BSP_UART_TJC_ReadStartCommand(void);

#endif