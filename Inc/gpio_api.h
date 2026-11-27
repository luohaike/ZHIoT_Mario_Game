#ifndef __gpio_api_H
#define __gpio_api_H
#include "stdint.h"
#include "gpio.h"

/*************************GPIO参数宏定义***************************************/
#define KEY1          0x01
#define KEY2          0x02
#define KEY3          0x04
#define KEY4          0x08
#define KEY_PRESS     0

#define LED1_PORT      LED1_Pin
#define LED2_PORT      LED2_Pin
#define LED3_PORT      LED3_Pin
#define LED4_PORT      LED4_Pin

#define LCD_LED_CLR     HAL_GPIO_WritePin(TFT_BL_GPIO_Port, TFT_BL_Pin, GPIO_PIN_RESET) //关背光 
#define LCD_LED_SET     HAL_GPIO_WritePin(TFT_BL_GPIO_Port, TFT_BL_Pin, GPIO_PIN_SET)   //开背光

#define LCD_CS_CLR      HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET) //片选置低
#define LCD_CS_SET      HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET)   //片选置高

#define LCD_RST_CLR     HAL_GPIO_WritePin(TFT_RESET_GPIO_Port,  TFT_RESET_Pin, GPIO_PIN_RESET)  //复位引脚置高
#define LCD_RST_SET     HAL_GPIO_WritePin(TFT_RESET_GPIO_Port,  TFT_RESET_Pin, GPIO_PIN_SET)    //复位引脚置低

#define LCD_RS_CLR      HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET)  //数据/命令引脚置高
#define LCD_RS_SET      HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET)    //数据/命令引脚置低

#define BC35G_PWR_H 	HAL_GPIO_WritePin(BC35G_SW_GPIO_Port, BC35G_SW_Pin, GPIO_PIN_RESET);    //给BC35模块上电
#define BC35G_PWR_L 	HAL_GPIO_WritePin(BC35G_SW_GPIO_Port, BC35G_SW_Pin, GPIO_PIN_SET);      //给BC35模块掉电

/***************************枚举变量定义***************************************/
typedef enum 
{
    BC35G_SW=0x01,
    BEEP=0x02,
    TFTS=0x04,
    LEDS=0x08,
    KEYS=0x10,
    E2PROOM=0x20
}GPIO_MODULE;

typedef enum 
{
    LED0=(uint16_t)0x0004,
    LED1=(uint16_t)0x0008,
    LED2=(uint16_t)0x0010,  
    LED3=(uint16_t)0x0020,     
}MODULE_LEDNUM;

typedef enum
{
    PIN_RESET = 0U,
    PIN_SET
}GPIO_PINSTATE;

/**************************外部函数声明****************************************/
void gpio_module_open(uint8_t module);
void gpio_module_close(uint8_t module);
void led_open(uint16_t lednum);
void led_close(uint16_t lednum);
void led_toggle(uint16_t lednum);
void beep_open(void);
void beep_close(void);
uint8_t keyscan(void);

#endif 
