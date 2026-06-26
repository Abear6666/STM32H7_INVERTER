#include "usbd_conf.h"

#include "main.h"
#include "usbd_cdc.h"
#include "usbd_core.h"

PCD_HandleTypeDef hpcd_USB_FS;

void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    GPIO_InitTypeDef gpio = {0};

    if (hpcd->Instance != USB_OTG_FS)
    {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USB2_OTG_FS_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF10_OTG1_FS;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(OTG_FS_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    if (hpcd->Instance != USB_OTG_FS)
    {
        return;
    }

    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
    __HAL_RCC_USB2_OTG_FS_CLK_DISABLE();
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SetupStage((USBD_HandleTypeDef *)hpcd->pData, (uint8_t *)hpcd->Setup);
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataOutStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->OUT_ep[epnum].xfer_buff);
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_DataInStage((USBD_HandleTypeDef *)hpcd->pData, epnum, hpcd->IN_ep[epnum].xfer_buff);
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_SOF((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_HandleTypeDef *pdev = (USBD_HandleTypeDef *)hpcd->pData;

    printf("USB CDC: bus reset\r\n");
    USBD_LL_Reset(pdev);
    USBD_LL_SetSpeed(pdev, USBD_SPEED_FULL);
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_Suspend((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd)
{
    USBD_LL_Resume((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum)
{
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef *)hpcd->pData, epnum);
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd)
{
    printf("USB CDC: connected\r\n");
    USBD_LL_DevConnected((USBD_HandleTypeDef *)hpcd->pData);
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd)
{
    printf("USB CDC: disconnected\r\n");
    USBD_LL_DevDisconnected((USBD_HandleTypeDef *)hpcd->pData);
}

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
    hpcd_USB_FS.Instance = USB_OTG_FS;
    hpcd_USB_FS.Init.dev_endpoints = 8U;
    hpcd_USB_FS.Init.use_dedicated_ep1 = 0U;
    hpcd_USB_FS.Init.ep0_mps = 0x40U;
    hpcd_USB_FS.Init.dma_enable = 0U;
    hpcd_USB_FS.Init.low_power_enable = 0U;
    hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_FS.Init.Sof_enable = 0U;
    hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_FS.Init.vbus_sensing_enable = 0U;
    hpcd_USB_FS.Init.lpm_enable = 0U;

    hpcd_USB_FS.pData = pdev;
    pdev->pData = &hpcd_USB_FS;

    if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
    {
        return USBD_FAIL;
    }

    HAL_PCDEx_SetRxFiFo(&hpcd_USB_FS, 0x80U);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 0U, 0x40U);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 1U, 0x40U);
    HAL_PCDEx_SetTxFiFo(&hpcd_USB_FS, 2U, 0x20U);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
    (void)HAL_PCD_DeInit((PCD_HandleTypeDef *)pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
    (void)HAL_PCD_Start((PCD_HandleTypeDef *)pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
    (void)HAL_PCD_Stop((PCD_HandleTypeDef *)pdev->pData);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t ep_type, uint16_t ep_mps)
{
    (void)HAL_PCD_EP_Open((PCD_HandleTypeDef *)pdev->pData, ep_addr, ep_mps, ep_type);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    (void)HAL_PCD_EP_Close((PCD_HandleTypeDef *)pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    (void)HAL_PCD_EP_Flush((PCD_HandleTypeDef *)pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    (void)HAL_PCD_EP_SetStall((PCD_HandleTypeDef *)pdev->pData, ep_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    (void)HAL_PCD_EP_ClrStall((PCD_HandleTypeDef *)pdev->pData, ep_addr);
    return USBD_OK;
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef *)pdev->pData;

    if ((ep_addr & 0x80U) != 0U)
    {
        return hpcd->IN_ep[ep_addr & 0x7FU].is_stall;
    }

    return hpcd->OUT_ep[ep_addr & 0x7FU].is_stall;
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev, uint8_t dev_addr)
{
    (void)HAL_PCD_SetAddress((PCD_HandleTypeDef *)pdev->pData, dev_addr);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    pdev->ep_in[ep_addr & 0x7FU].total_length = size;
    (void)HAL_PCD_EP_Transmit((PCD_HandleTypeDef *)pdev->pData, ep_addr, pbuf, size);
    return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev, uint8_t ep_addr, uint8_t *pbuf, uint32_t size)
{
    (void)HAL_PCD_EP_Receive((PCD_HandleTypeDef *)pdev->pData, ep_addr, pbuf, size);
    return USBD_OK;
}

uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef *)pdev->pData, ep_addr);
}

void *USBD_static_malloc(uint32_t size)
{
    static uint32_t mem[(sizeof(USBD_CDC_HandleTypeDef) / 4U) + 1U];

    (void)size;
    return mem;
}

void USBD_static_free(void *p)
{
    (void)p;
}

void USBD_LL_Delay(uint32_t delay)
{
    HAL_Delay(delay);
}
