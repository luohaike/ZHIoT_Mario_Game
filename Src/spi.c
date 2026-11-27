/*******************************************************************************
* @文件名        : spi
* @描述          : spi配置文件
* @作者          : 物联网项目组
* @日期          : 2020/09/04
* @版本          : V2.0.0
*******************************************************************************/
#include "spi.h"
#include "spi_api.h"
#include "string.h"
#include "stdio.h"

/**************************内部结构体声明**************************************/
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi3;

ZH_SPI_TypeDef midspi1;
ZH_SPI_TypeDef midspi2;
ZH_SPI_TypeDef midspi3;

/***************************内部函数声明***************************************/
static void MX_SPI1_Init(uint32_t BaudRate, SPI_Read_mode mode);
static void MX_SPI2_Init(uint32_t BaudRate, SPI_Read_mode mode);
static void MX_SPI3_Init(uint32_t BaudRate, SPI_Read_mode mode);
__weak void spi_error_handler(void);

/*******************************************************************************
* 名    称：spi_open
* 功    能：开启SPI及配置参数
* 参数说明：port：spi1/spi3/spi3
            BaudRate:spi波特率
            mode：Rising
                  Falling
* 返 回 值：无
* 说    明：开启SPI函数
*******************************************************************************/
void spi_open(SPI_port port, uint32_t BaudRate, SPI_Read_mode mode)
{
    if(port & spi1)
    {
        MX_SPI1_Init(BaudRate, mode);
        __HAL_SPI_ENABLE(&hspi1);

    }
    if(port & spi2)
    {
        MX_SPI2_Init(BaudRate, mode);
        __HAL_SPI_ENABLE(&hspi2);

    }
    if(port & spi3)
    {
        MX_SPI3_Init(BaudRate, mode);
        __HAL_SPI_ENABLE(&hspi3);

    }
}

/*******************************************************************************
* 名    称：spi_close
* 功    能：开启SPI及配置参数
* 参数说明：port：spi1/spi3/spi3
* 返 回 值：无
* 说    明：关闭SPI函数
*******************************************************************************/
void spi_close(SPI_port port)
{
    if(port & spi1)
    {
        __HAL_SPI_DISABLE(&hspi1);
        HAL_SPI_MspDeInit(&hspi1);
    }
    if(port & spi2)
    {
        __HAL_SPI_DISABLE(&hspi2);
        HAL_SPI_MspDeInit(&hspi2);
    }
    if(port & spi3)
    {
        __HAL_SPI_DISABLE(&hspi3);
        HAL_SPI_MspDeInit(&hspi3);
    }
}

/*******************************************************************************
* 名    称：spi_ReadWriteByte
* 功    能：开启SPI及配置参数
* 参数说明：port：spi1/spi3/spi3
            TxData：date
* 返 回 值：无
* 说    明：SPI读写函数
*******************************************************************************/
uint8_t spi_ReadWriteByte(SPI_port port, uint8_t TxData)
{
    if(port == spi1)
    {
        while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_TXE) == RESET );
        *((__IO uint8_t *)&hspi1.Instance->DR) = TxData;
        while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_RXNE) == RESET);
        return (uint8_t)hspi1.Instance->DR;
    }
    else if(port == spi2)
    {
        while (__HAL_SPI_GET_FLAG(&hspi2, SPI_FLAG_TXE) == RESET );
        *((__IO uint8_t *)&hspi2.Instance->DR) = TxData;
        while (__HAL_SPI_GET_FLAG(&hspi2, SPI_FLAG_RXNE) == RESET);
        return (uint8_t)hspi2.Instance->DR;
    }
    else if(port == spi3)
    {
        while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_TXE) == RESET );
        *((__IO uint8_t *)&hspi3.Instance->DR) = TxData;
        while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_RXNE) == RESET);
        return (uint8_t)hspi3.Instance->DR;
    }
    else
    {
        return 0;
    }
}

/*******************************************************************************
* 名    称：SPI_SetSpeed
* 功    能：设置SPI速度
* 参数说明：port：spi1/spi3/spi3
            SPI_BaudRatePrescaler：分频系数
* 返 回 值：无
* 说    明：设置SPI通信速率函数
*******************************************************************************/
void SPI_SetSpeed(SPI_port port, uint8_t SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler)); // 判断有效性
    if(port == spi1)
    {
        __HAL_SPI_DISABLE(&hspi1);            // 关闭SPI3
        hspi1.Instance->CR1 &= 0XFFC7;        // 位3-5清零，用来设置波特率
        hspi1.Instance->CR1 |= SPI_BaudRatePrescaler; //设置SPI波特率分频系数
        __HAL_SPI_ENABLE(&hspi1);             // 使能SPI
    }
    else if(port == spi2)
    {
        __HAL_SPI_DISABLE(&hspi2);            // 关闭SPI3
        hspi2.Instance->CR1 &= 0XFFC7;        // 位3-5清零，用来设置波特率
        hspi2.Instance->CR1 |= SPI_BaudRatePrescaler; //设置SPI波特率分频系数
        __HAL_SPI_ENABLE(&hspi2);             // 使能SPI
    }
    else if(port == spi3)
    {
        __HAL_SPI_DISABLE(&hspi3);            // 关闭SPI3
        hspi3.Instance->CR1 &= 0XFFC7;        // 位3-5清零，用来设置波特率
        hspi3.Instance->CR1 |= SPI_BaudRatePrescaler; //设置SPI波特率分频系数
        __HAL_SPI_ENABLE(&hspi3);             // 使能SPI
    }
}

/*******************************************************************************
* 名    称：MX_SPI1_Init
* 功    能：SPI1初始化函数
* 参数说明：BaudRate：波特率
            mode：Rising
                  Falling
* 返 回 值：无
* 说    明：SPI初始化函数
*******************************************************************************/
void MX_SPI1_Init(uint32_t BaudRate, SPI_Read_mode mode)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = BaudRate;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    if(mode == Rising)
    {
        hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    }
    else if(mode == Falling)
    {
        hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    }
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        spi_error_handler();
    }
}

/*******************************************************************************
* 名    称：MX_SPI2_Init
* 功    能：SPI2初始化函数
* 参数说明：BaudRate：波特率
            mode：Rising
                  Falling
* 返 回 值：无
* 说    明：SPI初始化函数
*******************************************************************************/
void MX_SPI2_Init(uint32_t BaudRate, SPI_Read_mode mode)
{
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = BaudRate;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 7;
    hspi2.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi2.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;

    if(mode == Rising)
    {
        hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    }
    else if(mode == Falling)
    {
        hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
    }
    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        spi_error_handler();
    }
}

/*******************************************************************************
* 名    称：MX_SPI3_Init
* 功    能：SPI3初始化函数
* 参数说明：BaudRate：波特率
            mode：Rising
                  Falling
* 返 回 值：无
* 说    明：SPI初始化函数
*******************************************************************************/
void MX_SPI3_Init(uint32_t BaudRate, SPI_Read_mode mode)
{
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.NSS = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = BaudRate;
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 7;
    hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi3.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

    if(mode == Rising)
    {
        hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    }
    else if(mode == Falling)
    {
        hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
    }

    if (HAL_SPI_Init(&hspi3) != HAL_OK)
    {
        spi_error_handler();
    }
}

/*******************************************************************************
* 名    称：HAL_SPI_MspInit
* 功    能：HAL库SPI内部初始化函数
* 参数说明：spiHandle：SPI结构体
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(spiHandle->Instance == SPI1)
    {
        __HAL_RCC_SPI1_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    }
    else if(spiHandle->Instance == SPI2)
    {
        __HAL_RCC_SPI2_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    }
    else if(spiHandle->Instance == SPI3)
    {
        __HAL_RCC_SPI3_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
}

/*******************************************************************************
* 名    称：HAL_SPI_MspDeInit
* 功    能：HAL库SPI内部清除函数
* 参数说明：spiHandle：SPI结构体
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle)
{
    if(spiHandle->Instance == SPI1)
    {
        __HAL_RCC_SPI1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
    }
    else if(spiHandle->Instance == SPI2)
    {
        __HAL_RCC_SPI2_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2 | GPIO_PIN_3);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_1);
    }
    else if(spiHandle->Instance == SPI3)
    {
        __HAL_RCC_SPI3_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
    }
}
