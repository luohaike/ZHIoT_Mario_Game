#ifndef __usart_H
#define __usart_H
#include "stm32l4xx_hal.h"

/************************UART参数宏定义****************************************/
#define UART_NUM             4
#define UART_RX_MAX_SIZE     200
#define UART_TX_MAX_SIZE     200
#define MAX_BAUDRATE         921600

/************************UART结构体声明****************************************/
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

uint32_t ig_lpuart1tick ;     
uint32_t ig_uart1tick ;       
uint32_t ig_uart2tick ;       
uint32_t ig_uart3tick ;  

/************************UART结构体定义****************************************/
typedef enum
{
    UART_STATE_CLOSED       = 0x00,
    UART_STATE_OPEN_IDLE    = 0x01,   
    UART_STATE_BUSY_TX      = 0x02,
    UART_STATE_BUSY_RX      = 0x03,
    UART_STATE_ERROR        = 0x04
}UART_StateTypeDef;

typedef struct 
{
    UART_StateTypeDef State;
    uint8_t prxdata[UART_RX_MAX_SIZE];
    uint32_t rxlen;
    uint8_t ptxdata[UART_TX_MAX_SIZE];
    uint32_t txlen;    
}UART_TypeDef;     

#endif
