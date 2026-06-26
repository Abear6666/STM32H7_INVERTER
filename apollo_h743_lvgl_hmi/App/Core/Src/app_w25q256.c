#include "app_w25q256.h"

#include <string.h>

#include "main.h"

#define W25Q_CMD_WRITE_ENABLE      0x06U
#define W25Q_CMD_READ_STATUS1      0x05U
#define W25Q_CMD_READ_DATA         0x03U
#define W25Q_CMD_PAGE_PROGRAM      0x02U
#define W25Q_CMD_SECTOR_ERASE      0x20U
#define W25Q_CMD_JEDEC_ID          0x9FU
#define W25Q_CMD_ENABLE_4BYTE_ADDR 0xB7U

#define W25Q_BUSY_MASK             0x01U

static QSPI_HandleTypeDef s_qspi;
static uint32_t s_jedec_id;

static bool app_w25q256_send_command(uint32_t instruction,
                                     uint32_t address,
                                     uint32_t address_mode,
                                     uint32_t address_size,
                                     uint32_t data_mode,
                                     uint32_t dummy_cycles,
                                     uint32_t length);
static bool app_w25q256_write_enable(void);
static bool app_w25q256_wait_ready(uint32_t timeout_ms);
static bool app_w25q256_read_status1(uint8_t *status);
static bool app_w25q256_enable_4byte_addr(void);
static bool app_w25q256_range_valid(uint32_t address, size_t length);

bool app_w25q256_init(void)
{
    uint8_t id[3] = {0};

    s_qspi.Instance = QUADSPI;
    s_qspi.Init.ClockPrescaler = 1;
    s_qspi.Init.FifoThreshold = 4;
    s_qspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    s_qspi.Init.FlashSize = 24;
    s_qspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_3_CYCLE;
    s_qspi.Init.ClockMode = QSPI_CLOCK_MODE_3;
    s_qspi.Init.FlashID = QSPI_FLASH_ID_1;
    s_qspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

    if (HAL_QSPI_Init(&s_qspi) != HAL_OK)
    {
        return false;
    }

    if (!app_w25q256_send_command(W25Q_CMD_JEDEC_ID,
                                  0,
                                  QSPI_ADDRESS_NONE,
                                  QSPI_ADDRESS_8_BITS,
                                  QSPI_DATA_1_LINE,
                                  0,
                                  sizeof(id)))
    {
        return false;
    }

    if (HAL_QSPI_Receive(&s_qspi, id, 1000) != HAL_OK)
    {
        return false;
    }

    s_jedec_id = ((uint32_t)id[0] << 16U) | ((uint32_t)id[1] << 8U) | id[2];
    if (id[2] != 0x19U)
    {
        return false;
    }

    return app_w25q256_enable_4byte_addr();
}

uint32_t app_w25q256_get_jedec_id(void)
{
    return s_jedec_id;
}

bool app_w25q256_read(uint32_t address, void *data, size_t length)
{
    if ((data == NULL) || !app_w25q256_range_valid(address, length))
    {
        return false;
    }

    if (length == 0U)
    {
        return true;
    }

    while (length > 0U)
    {
        uint32_t chunk = (length > 0xFFFFU) ? 0xFFFFU : (uint32_t)length;

        if (!app_w25q256_send_command(W25Q_CMD_READ_DATA,
                                      address,
                                      QSPI_ADDRESS_1_LINE,
                                      QSPI_ADDRESS_32_BITS,
                                      QSPI_DATA_1_LINE,
                                      0,
                                      chunk))
        {
            return false;
        }

        if (HAL_QSPI_Receive(&s_qspi, (uint8_t *)data, 1000) != HAL_OK)
        {
            return false;
        }

        address += chunk;
        data = (uint8_t *)data + chunk;
        length -= chunk;
    }

    return true;
}

bool app_w25q256_erase_sector(uint32_t address)
{
    address &= ~(APP_W25Q256_SECTOR_SIZE - 1UL);

    if (!app_w25q256_range_valid(address, APP_W25Q256_SECTOR_SIZE))
    {
        return false;
    }

    if (!app_w25q256_write_enable())
    {
        return false;
    }

    if (!app_w25q256_send_command(W25Q_CMD_SECTOR_ERASE,
                                  address,
                                  QSPI_ADDRESS_1_LINE,
                                  QSPI_ADDRESS_32_BITS,
                                  QSPI_DATA_NONE,
                                  0,
                                  0))
    {
        return false;
    }

    return app_w25q256_wait_ready(5000U);
}

bool app_w25q256_page_program(uint32_t address, const void *data, size_t length)
{
    if ((data == NULL) || (length == 0U) || (length > APP_W25Q256_PAGE_SIZE) ||
        (((address & (APP_W25Q256_PAGE_SIZE - 1UL)) + length) > APP_W25Q256_PAGE_SIZE) ||
        !app_w25q256_range_valid(address, length))
    {
        return false;
    }

    if (!app_w25q256_write_enable())
    {
        return false;
    }

    if (!app_w25q256_send_command(W25Q_CMD_PAGE_PROGRAM,
                                  address,
                                  QSPI_ADDRESS_1_LINE,
                                  QSPI_ADDRESS_32_BITS,
                                  QSPI_DATA_1_LINE,
                                  0,
                                  (uint32_t)length))
    {
        return false;
    }

    if (HAL_QSPI_Transmit(&s_qspi, (uint8_t *)data, 1000) != HAL_OK)
    {
        return false;
    }

    return app_w25q256_wait_ready(1000U);
}

bool app_w25q256_write_erased(uint32_t address, const void *data, size_t length)
{
    const uint8_t *src = (const uint8_t *)data;

    if ((data == NULL) || !app_w25q256_range_valid(address, length))
    {
        return false;
    }

    while (length > 0U)
    {
        size_t page_remaining = APP_W25Q256_PAGE_SIZE - (address & (APP_W25Q256_PAGE_SIZE - 1UL));
        size_t chunk = (length < page_remaining) ? length : page_remaining;

        if (!app_w25q256_page_program(address, src, chunk))
        {
            return false;
        }

        address += (uint32_t)chunk;
        src += chunk;
        length -= chunk;
    }

    return true;
}

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *hqspi)
{
    GPIO_InitTypeDef gpio = {0};

    if (hqspi->Instance != QUADSPI)
    {
        return;
    }

    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio.Pin = GPIO_PIN_6;
    gpio.Pull = GPIO_PULLUP;
    gpio.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_2;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOF, &gpio);

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOF, &gpio);
}

static bool app_w25q256_send_command(uint32_t instruction,
                                     uint32_t address,
                                     uint32_t address_mode,
                                     uint32_t address_size,
                                     uint32_t data_mode,
                                     uint32_t dummy_cycles,
                                     uint32_t length)
{
    QSPI_CommandTypeDef cmd;

    memset(&cmd, 0, sizeof(cmd));
    cmd.Instruction = instruction;
    cmd.Address = address;
    cmd.AddressSize = address_size;
    cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
    cmd.DummyCycles = dummy_cycles;
    cmd.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    cmd.AddressMode = address_mode;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = data_mode;
    cmd.NbData = length;
    cmd.DdrMode = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    return (HAL_QSPI_Command(&s_qspi, &cmd, 1000) == HAL_OK);
}

static bool app_w25q256_write_enable(void)
{
    if (!app_w25q256_send_command(W25Q_CMD_WRITE_ENABLE,
                                  0,
                                  QSPI_ADDRESS_NONE,
                                  QSPI_ADDRESS_8_BITS,
                                  QSPI_DATA_NONE,
                                  0,
                                  0))
    {
        return false;
    }

    return app_w25q256_wait_ready(1000U);
}

static bool app_w25q256_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint8_t status = 0xFFU;

    do
    {
        if (!app_w25q256_read_status1(&status))
        {
            return false;
        }

        if ((status & W25Q_BUSY_MASK) == 0U)
        {
            return true;
        }
    } while ((HAL_GetTick() - start) < timeout_ms);

    return false;
}

static bool app_w25q256_read_status1(uint8_t *status)
{
    if (status == NULL)
    {
        return false;
    }

    if (!app_w25q256_send_command(W25Q_CMD_READ_STATUS1,
                                  0,
                                  QSPI_ADDRESS_NONE,
                                  QSPI_ADDRESS_8_BITS,
                                  QSPI_DATA_1_LINE,
                                  0,
                                  1))
    {
        return false;
    }

    return (HAL_QSPI_Receive(&s_qspi, status, 1000) == HAL_OK);
}

static bool app_w25q256_enable_4byte_addr(void)
{
    if (!app_w25q256_write_enable())
    {
        return false;
    }

    if (!app_w25q256_send_command(W25Q_CMD_ENABLE_4BYTE_ADDR,
                                  0,
                                  QSPI_ADDRESS_NONE,
                                  QSPI_ADDRESS_8_BITS,
                                  QSPI_DATA_NONE,
                                  0,
                                  0))
    {
        return false;
    }

    return app_w25q256_wait_ready(1000U);
}

static bool app_w25q256_range_valid(uint32_t address, size_t length)
{
    if (length == 0U)
    {
        return true;
    }

    if (address >= APP_W25Q256_SIZE_BYTES)
    {
        return false;
    }

    return (length <= (APP_W25Q256_SIZE_BYTES - address));
}
