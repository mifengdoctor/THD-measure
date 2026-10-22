#include "adc_capture.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"

/*
 * ADC FIFO 一次向 DMA 提供 32 位数据，
 * 其中包含两个 16 位 ADC 样本。
 *
 * 这里的目标数组是 16 位数组，
 * DMA 按 32 位写入，因此数组必须 4 字节对齐。
 */
static uint16_t g_adc_samples[ADC_CAPTURE_SAMPLES]
    __attribute__((aligned(4)));

static volatile bool g_adc_done;

/*
 * FIFO 复位。
 *
 * 每次重新开始采集前执行，防止上一次采集结束后
 * FIFO 中残留半个 32 位数据，导致下一次样本错位。
 */
static void adc_fifo_restart(void)
{
    DL_ADC12_disableFIFO(ADC12_0_INST);
    DL_ADC12_enableFIFO(ADC12_0_INST);
}

static void adc_configure_dma(void)
{
    DL_DMA_setSrcAddr(
        DMA,
        DMA_CH0_CHAN_ID,
        (uint32_t)DL_ADC12_getFIFOAddress(ADC12_0_INST));

    DL_DMA_setDestAddr(
        DMA,
        DMA_CH0_CHAN_ID,
        (uint32_t)&g_adc_samples[0]);

    /*
     * 8192 个 16 位样本由 4096 个 32 位 FIFO Word 组成。
     */
    DL_DMA_setTransferSize(
        DMA,
        DMA_CH0_CHAN_ID,
        (uint16_t)(ADC_CAPTURE_SAMPLES / 2U));
}

void ADC_Capture_Init(void)
{
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

    adc_configure_dma();

    DL_ADC12_clearInterruptStatus(
        ADC12_0_INST,
        DL_ADC12_INTERRUPT_DMA_DONE);

    NVIC_ClearPendingIRQ(ADC12_0_INST_INT_IRQN);
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);

    g_adc_done = false;
}

bool ADC_Capture_Start(void)
{
    g_adc_done = false;

    /*
     * 先停止触发源和 DMA。
     */
    DL_TimerG_stopCounter(TIMER_0_INST);
    DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

    /*
     * ADC DMA DONE 后，ADC 硬件会把 DMAEN 清零，
     * 因此下一轮采集前必须由软件重新使能。
     */
    DL_ADC12_disableDMA(ADC12_0_INST);

    /*
     * 等待可能正在进行的最后一次转换完成。
     * 32 MHz 下大约 1 us。
     */
    delay_cycles(CPUCLK_FREQ / 1000000U);

    /*
     * 清 FIFO，并重新使能 ADC 的 DMA 功能。
     */
    adc_fifo_restart();
    DL_ADC12_enableConversions(ADC12_0_INST);

    /*
     * 重新设置 DMA 地址和传输长度。
     */
    adc_configure_dma();

    /*
     * 清除上一次的 ADC DMA DONE 和 DMA 触发标志。
     */
    DL_ADC12_clearInterruptStatus(
        ADC12_0_INST,
        DL_ADC12_INTERRUPT_DMA_DONE);

    DL_ADC12_clearDMATriggerStatus(
    ADC12_0_INST,
    DL_ADC12_DMA_MEM10_RESULT_LOADED);
    
    NVIC_ClearPendingIRQ(ADC12_0_INST_INT_IRQN);

    /*
     * 先让 ADC 可以产生 DMA 请求，
     * 再打开 DMA 通道，最后启动定时器。
     */
    DL_ADC12_enableDMA(ADC12_0_INST);
    DL_DMA_enableChannel(DMA, DMA_CH0_CHAN_ID);

    DL_TimerG_startCounter(TIMER_0_INST);

    return true;
}

const uint16_t *ADC_Capture_GetSamples(void)
{
    return g_adc_samples;
}

bool ADC_Capture_IsDone(void)
{
    return g_adc_done;
}

void ADC12_0_INST_IRQHandler(void)
{
    if (DL_ADC12_getPendingInterrupt(ADC12_0_INST) ==
        DL_ADC12_IIDX_DMA_DONE) {

        /*
         * 停止定时器，停止后续触发。
         */
        DL_TimerG_stopCounter(TIMER_0_INST);

        /*
         * 关闭 DMA 通道。
         * ADC 的 DMAEN 已经由硬件清除，下一轮开始时会重新使能。
         */
        DL_DMA_disableChannel(DMA, DMA_CH0_CHAN_ID);

        g_adc_done = true;
    }
}