/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-06
 * @brief       QSPI 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 阿波罗 H743开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/SDRAM/sdram.h"
#include "./USMART/usmart.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/MPU/mpu.h"
#include "./BSP/QSPI/qspi.h"
#include "./BSP/NORFLASH/norflash.h"


/* 要写入到FLASH的字符串数组 */
const uint8_t g_text_buf[] = {"STM32H7 QSPI TEST"};

#define TEXT_SIZE       sizeof(g_text_buf)  /* TEXT字符串长度 */
extern uint16_t g_norflash_type;            /* 默认是W25Q256 */

int main(void)
{
    uint8_t key;
    uint16_t i = 0;
    uint8_t datatemp[TEXT_SIZE + 2];
    uint8_t rectemp[TEXT_SIZE + 2];
    uint32_t flashsize;

    sys_cache_enable();                     /* 打开L1-Cache */
    HAL_Init();                             /* 初始化HAL库 */
    sys_stm32_clock_init(160, 5, 2, 4);     /* 设置时钟, 400Mhz */
    delay_init(400);                        /* 延时初始化 */
    usart_init(115200);                     /* 串口初始化 */
    usmart_init(200);                       /* 初始化USMART */
    mpu_memory_protection();                /* 保护相关存储区域 */
    led_init();                             /* 初始化LED */
    key_init();                             /* 初始化KEY */
    sdram_init();                           /* 初始化SDRAM */
    lcd_init();                             /* 初始化LCD */
    norflash_init();                        /* 初始化norflash */

    lcd_show_string(30, 50, 200, 16, 16, "STM32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "QSPI TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, "KEY1:Write  KEY0:Read", RED);    /* 显示提示信息 */

    while (norflash_read_id() != g_norflash_type)              /* 检测不到FLASH芯片 */
    {
        lcd_show_string(30, 130, 200, 16, 16, "FLASH Check Failed!", RED);
        delay_ms(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check!      ", RED);
        delay_ms(500);
        LED0_TOGGLE();                                        /* LED0闪烁 */
    }

    lcd_show_string(30, 130, 200, 16, 16, "QSPI FLASH Ready!", BLUE);
    flashsize = 32 * 1024 * 1024;                             /* FLASH 大小为32M字节 */

    while (1)
    {
        key = key_scan(0);

        if (key == KEY1_PRES)           /* KEY1按下,写入 */
        {
            lcd_fill(0, 150, 239, 319, WHITE);  /* 清除半屏 */
            lcd_show_string(30, 150, 200, 16, 16, "Start Write FLASH....", BLUE);
            sprintf((char *)datatemp, "%s%d", (char *)g_text_buf, i);
            norflash_write((uint8_t *)datatemp, flashsize - 200, TEXT_SIZE + 2);   /* 从倒数第200个地址处开始,写入SIZE长度的数据 */
            lcd_show_string(30, 150, 200, 16, 16, "FLASH Write Finished!", BLUE);  /* 提示传送完成 */
            printf("datatemp:%s  \r\n", datatemp);
        }

        if (key == KEY0_PRES)           /* KEY0按下,读取字符串并显示 */
        {
            lcd_show_string(30, 150, 200, 16, 16, "Start Read FLASH... . ", BLUE);
            norflash_read((uint8_t *)rectemp, flashsize - 200, TEXT_SIZE + 2);     /* 从倒数第200个地址处开始,读出SIZE个字节 */
            lcd_show_string(30, 150, 200, 16, 16, "The Data Readed Is:   ", BLUE); /* 提示传送完成 */
            lcd_show_string(30, 170, 200, 16, 16, (char *)rectemp, BLUE);          /* 显示读到的字符串 */
            printf("rectemp:%s  \r\n", rectemp);
        }

        i++;

        if (i == 20)
        {
            LED0_TOGGLE();              /* LED0闪烁 */
            i = 0;
        }

        delay_ms(10);
    }
}
