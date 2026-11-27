/**
 * @file    syscalls.c
 * @brief   GCC (picolibc / newlib) printf retargeting over USART2 for STM32L431.
 */
#include <stdio.h>
#include "stm32l4xx_hal.h"

extern UART_HandleTypeDef huart2;

static int uart_putchar(uint8_t ch)
{
    HAL_UART_Transmit(&huart2, &ch, 1, 0xFFFF);
    return (int)ch;
}

#if defined(__PICOLIBC__)
static int picolibc_putc(char c, FILE *file)
{
    (void)file;
    return uart_putchar((uint8_t)c);
}

static FILE __stdio = FDEV_SETUP_STREAM(picolibc_putc, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdout = &__stdio;
FILE *const stderr = &__stdio;
#elif defined(__GNUC__)
int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++) {
        uart_putchar((uint8_t)ptr[i]);
    }
    return len;
}
#endif
