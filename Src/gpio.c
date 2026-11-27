/*******************************************************************************
* @文件名        : gpio
* @描述          : GPIO引脚配置文件
* @作者          : 物联网项目组
* @日期          : 2020/09/04
* @版本          : V2.0.0
*******************************************************************************/
#include "gpio.h"
#include "gpio_api.h"

/*******************************************************************************
* 名    称：gpio_module_open
* 功    能：初始化各硬件引脚的IO口
* 参数说明：module：要开启硬件引脚的IO口
* 返 回 值：无
* 说    明：包含BC35、BEEP、E2PROOM、TFT、LED、KEY的引脚控制
*******************************************************************************/
void gpio_module_open(uint8_t module)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(module & BC35G_SW)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        HAL_GPIO_WritePin(BC35G_SW_GPIO_Port, BC35G_SW_Pin, GPIO_PIN_SET);
        GPIO_InitStruct.Pin = BC35G_SW_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(BC35G_SW_GPIO_Port, &GPIO_InitStruct);
    }
    if(module & BEEP)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
        GPIO_InitStruct.Pin = BEEP_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(BEEP_GPIO_Port, &GPIO_InitStruct);
    }
    if(module & E2PROOM)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        HAL_GPIO_WritePin(W25Q128_GPIO_Port, W25Q128_Pin, GPIO_PIN_RESET);
        GPIO_InitStruct.Pin = W25Q128_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(W25Q128_GPIO_Port, &GPIO_InitStruct);
    }
    if(module & TFTS)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        HAL_GPIO_WritePin(GPIOE, TFT_DC_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOE, TFT_CS_Pin | TFT_RESET_Pin | TFT_BL_Pin, GPIO_PIN_SET);
        GPIO_InitStruct.Pin = TFT_CS_Pin | TFT_RESET_Pin | TFT_BL_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = TFT_DC_Pin;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
    if(module & LEDS)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin | LED2_Pin | LED3_Pin | LED4_Pin, GPIO_PIN_SET);
        GPIO_InitStruct.Pin = LED1_Pin | LED2_Pin | LED3_Pin | LED4_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(LED1_GPIO_Port, &GPIO_InitStruct);
    }
    if(module & KEYS)
    {
        __HAL_RCC_GPIOD_CLK_ENABLE();
        GPIO_InitStruct.Pin = KEY1_Pin | KEY2_Pin | KEY3_Pin | KEY4_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);
    }
}

/*******************************************************************************
* 名    称：gpio_module_close
* 功    能：关闭各硬件引脚的IO口
* 参数说明：module：关闭硬件引脚的IO口
* 返 回 值：无
* 说    明：包含BC35、BEEP、E2PROOM、TFT、LED、KEY的引脚控制
*******************************************************************************/
void gpio_module_close(uint8_t module)
{
    if(module & BC35G_SW)
    {
        HAL_GPIO_DeInit(BC35G_SW_GPIO_Port, BC35G_SW_Pin);
    }
    if(module & BEEP)
    {
        HAL_GPIO_DeInit(BEEP_GPIO_Port, BEEP_Pin);
    }
    if(module & TFTS)
    {
        HAL_GPIO_DeInit(GPIOE, TFT_CS_Pin | TFT_RESET_Pin | TFT_BL_Pin | TFT_DC_Pin);
    }
    if(module & LEDS)
    {
        HAL_GPIO_DeInit(LED1_GPIO_Port, LED1_Pin | LED2_Pin | LED3_Pin | LED4_Pin);
    }
    if(module & KEYS)
    {
        HAL_GPIO_DeInit(KEY1_GPIO_Port, KEY1_Pin | KEY2_Pin | KEY3_Pin | KEY4_Pin);
    }
}

/*******************************************************************************
* 名    称：led_open
* 功    能：开启LED
* 参数说明：lednum：LED1_Pin
                    LED2_Pin
                    LED3_Pin
                    LED4_Pin
* 返 回 值：无
* 说    明：低电平点亮
*******************************************************************************/
void led_open(uint16_t lednum)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, lednum, GPIO_PIN_RESET);
}

/*******************************************************************************
* 名    称：led_close
* 功    能：关闭LED
* 参数说明：lednum：LED1_Pin
                    LED2_Pin
                    LED3_Pin
                    LED4_Pin
* 返 回 值：无
* 说    明：高电平关闭
*******************************************************************************/
void led_close(uint16_t lednum)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, lednum, GPIO_PIN_SET);
}

/*******************************************************************************
* 名    称：led_toggle
* 功    能：LED状态翻转
* 参数说明：lednum：LED1_Pin
                    LED2_Pin
                    LED3_Pin
                    LED4_Pin
* 返 回 值：无
* 说    明：翻转一次
*******************************************************************************/
void led_toggle(uint16_t lednum)
{
    HAL_GPIO_TogglePin(LED1_GPIO_Port, lednum);
}

/*******************************************************************************
* 名    称：beep_open
* 功    能：打开蜂鸣器
* 参数说明：无
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void beep_open(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);
}

/*******************************************************************************
* 名    称：beep_close
* 功    能：关闭蜂鸣器
* 参数说明：无
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void beep_close(void)
{
    HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_RESET);
}

/*******************************************************************************
* 名    称：keyscan
* 功    能：按键检测函数
* 参数说明：无
* 返 回 值：无
* 说    明：无
*******************************************************************************/
uint8_t keyscan(void)
{
    uint8_t keynum = 0;

    if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == KEY_PRESS)
    {
        keynum += KEY1;
    }

    if(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == KEY_PRESS)
    {
        keynum += KEY2;
    }

    if(HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == KEY_PRESS)
    {
        keynum += KEY3;
    }

    if(HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == KEY_PRESS)
    {
        keynum += KEY4;
    }
    return keynum;
}
