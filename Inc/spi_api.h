#ifndef __spi_api_H
#define __spi_api_H
#include "stdint.h"

/*************************SPI枚举变量定义**************************************/	
typedef enum 
{
    spi1=0x01,
    spi2=0x02,
    spi3=0x04
}SPI_port;  

typedef enum 
{
    Rising=0,
    Falling
}SPI_Read_mode; 

/**************************SPI函数声明****************************************/	
void spi_open(SPI_port port,uint32_t BaudRate,SPI_Read_mode mode); 
void spi_close(SPI_port port);
void spi_error_handler(void);
uint8_t spi_ReadWriteByte(SPI_port port,uint8_t TxData);

#endif 

