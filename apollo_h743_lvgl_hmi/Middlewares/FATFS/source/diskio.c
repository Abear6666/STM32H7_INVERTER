#include "ff.h"
#include "diskio.h"

#include <stdio.h>

#include "app_sd.h"

#define DEV_SD 0U

DSTATUS disk_status(BYTE pdrv)
{
    bool ready;

    if (pdrv != DEV_SD)
    {
        printf("disk_status: invalid pdrv=%u\r\n", (unsigned int)pdrv);
        return STA_NOINIT;
    }

    ready = app_sd_is_card_ready();
    if (!ready)
    {
        printf("disk_status: SD not ready err=0x%08lX state=%lu\r\n",
               (unsigned long)app_sd_get_last_error(),
               (unsigned long)app_sd_get_card_state());
    }

    return ready ? 0U : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    bool ok;

    if (pdrv != DEV_SD)
    {
        printf("disk_initialize: invalid pdrv=%u\r\n", (unsigned int)pdrv);
        return STA_NOINIT;
    }

    ok = app_sd_initialize();
    if (!ok)
    {
        printf("disk_initialize: SD init failed err=0x%08lX state=%lu\r\n",
               (unsigned long)app_sd_get_last_error(),
               (unsigned long)app_sd_get_card_state());
    }

    return ok ? 0U : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    bool ok;

    if ((pdrv != DEV_SD) || (buff == NULL) || (count == 0U))
    {
        printf("disk_read: bad args pdrv=%u buff=%p sector=%lu count=%u\r\n",
               (unsigned int)pdrv,
               (void *)buff,
               (unsigned long)sector,
               (unsigned int)count);
        return RES_PARERR;
    }

    ok = app_sd_read_blocks(buff, (uint32_t)sector, (uint32_t)count);
    if (!ok)
    {
        printf("disk_read: SD read failed sector=%lu count=%u err=0x%08lX state=%lu\r\n",
               (unsigned long)sector,
               (unsigned int)count,
               (unsigned long)app_sd_get_last_error(),
               (unsigned long)app_sd_get_card_state());
    }

    return ok ? RES_OK : RES_ERROR;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if ((pdrv != DEV_SD) || (buff == NULL) || (count == 0U))
    {
        return RES_PARERR;
    }

    return app_sd_write_blocks(buff, (uint32_t)sector, (uint32_t)count) ? RES_OK : RES_ERROR;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    HAL_SD_CardInfoTypeDef info;

    if (pdrv != DEV_SD)
    {
        printf("disk_ioctl: invalid pdrv=%u cmd=%u\r\n", (unsigned int)pdrv, (unsigned int)cmd);
        return RES_PARERR;
    }

    switch (cmd)
    {
    case CTRL_SYNC:
        return RES_OK;

    case GET_SECTOR_SIZE:
        if (buff == NULL)
        {
            return RES_PARERR;
        }
        *(WORD *)buff = 512U;
        return RES_OK;

    case GET_BLOCK_SIZE:
        if (buff == NULL)
        {
            return RES_PARERR;
        }
        *(DWORD *)buff = 1U;
        return RES_OK;

    case GET_SECTOR_COUNT:
        if ((buff == NULL) || !app_sd_get_card_info(&info))
        {
            printf("disk_ioctl: GET_SECTOR_COUNT failed buff=%p err=0x%08lX state=%lu\r\n",
                   buff,
                   (unsigned long)app_sd_get_last_error(),
                   (unsigned long)app_sd_get_card_state());
            return RES_ERROR;
        }
        *(DWORD *)buff = info.LogBlockNbr;
        return RES_OK;

    default:
        printf("disk_ioctl: unsupported cmd=%u\r\n", (unsigned int)cmd);
        return RES_PARERR;
    }
}
