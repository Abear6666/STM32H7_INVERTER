#include "uart.h"

#include <sys/unistd.h>

#ifdef APP_USE_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif

#ifdef APP_USB_CDC_ENABLED
#include "app_usb_cdc.h"
#endif
#include "main.h"

#define UART1_RX_RING_SIZE 2048U

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

static uint8_t s_uart1_rx_byte;
static volatile uint8_t s_uart1_rx_ring[UART1_RX_RING_SIZE];
static volatile uint16_t s_uart1_rx_head;
static volatile uint16_t s_uart1_rx_tail;
#ifdef APP_USE_FREERTOS
static SemaphoreHandle_t s_uart_tx_mutex;
#endif

static void app_uart1_rx_rearm(void);

void app_uart1_init(uint32_t baudrate)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = baudrate;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

void app_uart_log_mutex_init(void)
{
#ifdef APP_USE_FREERTOS
    if (s_uart_tx_mutex == NULL)
    {
        s_uart_tx_mutex = xSemaphoreCreateMutex();
    }
#endif
}

void app_uart1_rx_start_it(void)
{
    s_uart1_rx_head = 0;
    s_uart1_rx_tail = 0;
    app_uart1_rx_rearm();
}

bool app_uart1_rx_read_byte(uint8_t *byte)
{
    uint16_t tail;

    if (byte == NULL)
    {
        return false;
    }

    __disable_irq();
    tail = s_uart1_rx_tail;
    if (tail == s_uart1_rx_head)
    {
        __enable_irq();
        return false;
    }

    *byte = s_uart1_rx_ring[tail];
    s_uart1_rx_tail = (uint16_t)((tail + 1U) % UART1_RX_RING_SIZE);
    __enable_irq();
    return true;
}

void app_uart2_init(uint32_t baudrate)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = baudrate;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
}

bool app_uart2_rx_start_dma(uint8_t *buffer, uint16_t size)
{
    HAL_StatusTypeDef status;

    if ((buffer == NULL) || (size == 0U))
    {
        return false;
    }

    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF | UART_CLEAR_IDLEF);
    status = HAL_UARTEx_ReceiveToIdle_DMA(&huart2, buffer, size);
    if (status == HAL_OK)
    {
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
        return true;
    }

    return false;
}

__attribute__((weak)) void app_uart2_rx_on_idle_from_isr(uint16_t size)
{
    (void)size;
}

__attribute__((weak)) void app_uart2_rx_on_error_from_isr(uint32_t error_code)
{
    (void)error_code;
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};

    if (huart->Instance == USART1)
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_PULLUP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &gpio);

        HAL_NVIC_SetPriority(USART1_IRQn, 3, 3);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        return;
    }

    if (huart->Instance == USART2)
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_PULLUP;
        gpio.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &gpio);

        hdma_usart2_rx.Instance = DMA1_Stream0;
        hdma_usart2_rx.Init.Request = DMA_REQUEST_USART2_RX;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_NORMAL;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmarx, hdma_usart2_rx);

        HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

        HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        return;
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint16_t next;

    if (huart->Instance == USART1)
    {
        next = (uint16_t)((s_uart1_rx_head + 1U) % UART1_RX_RING_SIZE);
        if (next != s_uart1_rx_tail)
        {
            s_uart1_rx_ring[s_uart1_rx_head] = s_uart1_rx_byte;
            s_uart1_rx_head = next;
        }

        app_uart1_rx_rearm();
        return;
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2)
    {
        app_uart2_rx_on_idle_from_isr(Size);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        app_uart1_rx_rearm();
    }
    else if (huart->Instance == USART2)
    {
        app_uart2_rx_on_error_from_isr(huart->ErrorCode);
    }
}

int _write(int file, char *ptr, int len)
{
#ifdef APP_USE_FREERTOS
    bool use_mutex;
#endif

    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        return -1;
    }

#ifdef APP_USE_FREERTOS
    use_mutex = (s_uart_tx_mutex != NULL) &&
                (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) &&
                (__get_IPSR() == 0U);

    if (use_mutex)
    {
        (void)xSemaphoreTake(s_uart_tx_mutex, portMAX_DELAY);
    }
#endif

    if (HAL_UART_Transmit(&huart1, (uint8_t *)ptr, (uint16_t)len, HAL_MAX_DELAY) != HAL_OK)
    {
#ifdef APP_USE_FREERTOS
        if (use_mutex)
        {
            (void)xSemaphoreGive(s_uart_tx_mutex);
        }
#endif
        return -1;
    }

#ifdef APP_USE_FREERTOS
    if (use_mutex)
    {
        (void)xSemaphoreGive(s_uart_tx_mutex);
    }
#endif

    return len;
}

static void app_uart1_rx_rearm(void)
{
    (void)HAL_UART_Receive_IT(&huart1, &s_uart1_rx_byte, 1);
}
