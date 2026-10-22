#include "bsp_uart_tjc.h"

#include "ti_msp_dl_config.h"

#define RX_RING_SIZE       (128U)
#define RX_RING_MASK       (RX_RING_SIZE - 1U)

#define TJC_START_COMMAND_LENGTH    (7U)

static volatile uint8_t g_rx_buffer[RX_RING_SIZE];
static volatile uint16_t g_rx_head;
static volatile uint16_t g_rx_tail;

static uint8_t g_start_match_index;

static const uint8_t g_start_command[TJC_START_COMMAND_LENGTH] =
{
    0x55U,
    0x10U,
    0x00U,
    0x00U,
    0xFFU,
    0xFFU,
    0xFFU
};

void BSP_UART_TJC_Init(void)
{
    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_start_match_index = 0U;

    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
}

void BSP_UART_TJC_SendByte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, data);
}

void BSP_UART_TJC_SendString(const char *text)
{
    while (*text != '\0') {
        BSP_UART_TJC_SendByte((uint8_t)*text);
        text++;
    }
}

void BSP_UART_TJC_SendBytes(
    const uint8_t *data,
    uint16_t length)
{
    uint16_t i;

    for (i = 0U; i < length; i++) {
        BSP_UART_TJC_SendByte(data[i]);
    }
}

bool BSP_UART_TJC_ReadByte(uint8_t *data)
{
    uint32_t head;
    uint32_t tail;

    head = (uint32_t)g_rx_head;
    tail = (uint32_t)g_rx_tail;

    if (head == tail) {
        return false;
    }

    *data = g_rx_buffer[head];
    g_rx_head = (uint16_t)((head + 1U) & RX_RING_MASK);

    return true;
}

bool BSP_UART_TJC_ReadStartCommand(void)
{
    uint8_t data;

    while (BSP_UART_TJC_ReadByte(&data)) {

        if (data == g_start_command[g_start_match_index]) {
            g_start_match_index++;

            if (g_start_match_index >= TJC_START_COMMAND_LENGTH) {
                g_start_match_index = 0U;
                return true;
            }
        } else {
            if (data == g_start_command[0]) {
                g_start_match_index = 1U;
            } else {
                g_start_match_index = 0U;
            }
        }
    }

    return false;
}

void UART_0_INST_IRQHandler(void)
{
    DL_UART_IIDX pending;

    pending = DL_UART_Main_getPendingInterrupt(UART_0_INST);

    if (pending == DL_UART_MAIN_IIDX_RX) {
        uint8_t data;
        uint16_t next_tail;

        data = (uint8_t)DL_UART_Main_receiveData(UART_0_INST);

        next_tail = (uint16_t)(
            ((uint32_t)g_rx_tail + 1U) & RX_RING_MASK);

        if (next_tail != g_rx_head) {
            g_rx_buffer[g_rx_tail] = data;
            g_rx_tail = next_tail;
        }
    }
}