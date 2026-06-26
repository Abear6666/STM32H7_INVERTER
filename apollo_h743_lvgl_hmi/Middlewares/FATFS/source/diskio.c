#include "ff.h"
#include "diskio.h"

#include "app_sd.h"

#define DEV_SD 0U

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != DEV_SD)
    {
        return STA_NOINIT;
    }

    return app_sd_is_card_ready() ? 0U : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != DEV_SD)
    {
        return STA_NOINIT;
    }

    return app_sd_initialize() ? 0U : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if ((pdrv != DEV_SD) || (buff == NULL) || (count == 0U))
    {
        return RES_PARERR;
    }

    return app_sd_read_blocks(buff, (uint32_t)sector, (uint32_t)count) ? RES_OK : RES_ERROR;
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
            return RES_ERROR;
        }
        *(DWORD *)buff = info.LogBlockNbr;
        return RES_OK;

    default:
        return RES_PARERR;
    }
}
