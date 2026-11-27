#ifndef __usart_api_H
#define __usart_api_H
#include "stdint.h"
#include "stdio.h"

/************************UART枚举变量定义**************************************/
typedef enum 
{
    lpuart1=1,
    uart1=2,
    uart2=3,
    uart3=4
}UART_port;    

/************************UART外部函数声明**************************************/
void uart_open(UART_port port,uint32_t BaudRate); 
void uart_close(UART_port port); 
void clearrxbuffer(UART_port port);
void uart_write(UART_port port,uint16_t length,uint8_t* ptrdata);
void uart_error_handle(void);
uint8_t * uart_read(UART_port port,uint16_t *length); 
int fputc(int ch, FILE *f);

#endif 
