#ifndef __gpio_H
#define __gpio_H

#include "stm32l4xx_hal.h"

/*************************GPIO引脚宏定义***************************************/	
#define BEEP_Pin            GPIO_PIN_8
#define BEEP_GPIO_Port      GPIOE

#define W25Q128_Pin         GPIO_PIN_6
#define W25Q128_GPIO_Port   GPIOE

#define BC35G_SW_Pin        GPIO_PIN_1
#define BC35G_SW_GPIO_Port  GPIOA

#define KEY1_Pin            GPIO_PIN_5
#define KEY1_GPIO_Port      GPIOD
#define KEY2_Pin            GPIO_PIN_4
#define KEY2_GPIO_Port      GPIOD
#define KEY3_Pin            GPIO_PIN_3
#define KEY3_GPIO_Port      GPIOD
#define KEY4_Pin            GPIO_PIN_2
#define KEY4_GPIO_Port      GPIOD

#define TFT_CS_Pin          GPIO_PIN_9
#define TFT_CS_GPIO_Port    GPIOE
#define TFT_RESET_Pin       GPIO_PIN_10
#define TFT_RESET_GPIO_Port GPIOE
#define TFT_DC_Pin          GPIO_PIN_11
#define TFT_DC_GPIO_Port    GPIOE
#define TFT_BL_Pin          GPIO_PIN_12
#define TFT_BL_GPIO_Port    GPIOE

#define LED1_Pin            GPIO_PIN_4
#define LED1_GPIO_Port      GPIOA
#define LED2_Pin            GPIO_PIN_5
#define LED2_GPIO_Port      GPIOA
#define LED3_Pin            GPIO_PIN_6
#define LED3_GPIO_Port      GPIOA
#define LED4_Pin            GPIO_PIN_7
#define LED4_GPIO_Port      GPIOA

#endif 
