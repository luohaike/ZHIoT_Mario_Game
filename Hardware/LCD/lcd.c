/*------------------------------------------------------------------------------
* @文件名        : lcd
* @描述          : TFT屏基础配置相关函数
* @作者          : 物联网项目组
* @日期          : 2023/04/01
* @版本          : V0.0.2
********************************************************************************
本驱动函数使用指导流程：
1、添加本驱动的.c和.h文件
2、使用lcd来显示printf的内容
*******************************************************************************/
#include "stm32l4xx_hal.h"
#include "stdlib.h"
#include "spi_api.h" 
#include "gpio_api.h"
#include "lcd.h"

extern SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;
static volatile uint8_t lcd_dma_busy = 0U;
static uint8_t lcd_dma_inited = 0U;

/***************************内部参数宏定义*************************************/	
#define  SPI_WriteByte(data)    spi_ReadWriteByte(spi1,data)
#define  SPI_GPIO_Init(void)    spi_open(spi1,SPI_BAUDRATEPRESCALER_2,Rising)
#define  LCD_GPIO_init()        gpio_module_open(TFTS);	
#define  Byte_to_Word(byte)	    ((byte[1]&0xff)<<8)|(byte[0]&0xff);
	   
/*****************************全局变量定义*************************************/	
_lcd_dev lcddev; //管理LCD重要参数
uint16_t POINT_COLOR = 0x0000; //画笔颜色,
uint16_t BACK_COLOR = 0xFFFF;  //背景颜色

/*****************************内部函数声明*************************************/	
static void LCD_WR_REG(uint8_t data);
static void LCD_WR_DATA(uint8_t data);
static void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue);
static void LCD_WriteRAM_Prepare(void); 
static void LCD_RESET(void);

/*******************************************************************************
* 名    称：LCD_WR_REG
* 功    能：LCD写寄存器函数
* 参数说明：data：写入寄存器的数据
* 返 回 值：无
* 说    明：无
*******************************************************************************/
static void LCD_WR_REG(uint8_t data)
{ 
   LCD_CS_CLR;     
   LCD_RS_CLR;	  
   SPI_WriteByte(data);
   LCD_CS_SET;	
}

/*******************************************************************************
* 名    称：LCD_WR_DATA
* 功    能：写数据函数
* 参数说明：data：要写入的数据
* 返 回 值：无
* 说    明：无
*******************************************************************************/
static void LCD_WR_DATA(uint8_t data)
{
    LCD_CS_CLR;
    LCD_RS_SET;
    SPI_WriteByte(data);
    LCD_CS_SET;
}

/*******************************************************************************
* 名    称：LCD_WriteReg
* 功    能：往寄存器里写入数据
* 参数说明：LCD_Reg:寄存器地址
            LCD_RegValue：数据
* 返 回 值：无
* 说    明：无
*******************************************************************************/
static void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{	
	LCD_WR_REG(LCD_Reg);  
	LCD_WR_DATA(LCD_RegValue);	    		 
}
 
/*******************************************************************************
* 名    称：Lcd_WriteData_16Bit
* 功    能：写入16位数据
* 参数说明：Data：16位数据
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void Lcd_WriteData_16Bit(uint16_t Data)
{	
    LCD_CS_CLR;
    LCD_RS_SET;  
    SPI_WriteByte(Data>>8);
    SPI_WriteByte(Data);
    LCD_CS_SET;
}

/*******************************************************************************
* 名    称：LCD_WriteRAM_Prepare
* 功    能：写RAM函数
* 参数说明：无
* 返 回 值：无
* 说    明：无
*******************************************************************************/ 
static void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
}	 

/*******************************************************************************
* 名    称：LCD_DrawPoint
* 功    能：画点函数
* 参数说明：x:x轴的位置
            y:y轴的位置
* 返 回 值：无
* 说    明：固定颜色
*******************************************************************************/ 
void LCD_DrawPoint(uint16_t x,uint16_t y)
{
	LCD_SetCursor(x,y);     //设置光标位置 
	Lcd_WriteData_16Bit(POINT_COLOR); 
}

/*******************************************************************************
* 名    称：LCD_DrawPoint1
* 功    能：画点函数
* 参数说明：x:x轴的位置
            y:y轴的位置
            color:颜色
* 返 回 值：无
* 说    明：无
*******************************************************************************/ 
void LCD_DrawPoint1(uint16_t x,uint16_t y,uint16_t color)
{
	LCD_SetCursor(x,y);//设置光标位置 
	Lcd_WriteData_16Bit(color); 
}

/*******************************************************************************
* 名    称：LCD_Clear
* 功    能：清屏函数
* 参数说明：Color:清屏后的颜色
* 返 回 值：无
* 说    明：无
*******************************************************************************/ 
void LCD_Clear(uint16_t Color)
{
    unsigned int i,m;  
	LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);   
	LCD_CS_CLR;
	LCD_RS_SET;
	for(i=0;i<lcddev.height;i++)
	{
        for(m=0;m<lcddev.width;m++)
        {	
			Lcd_WriteData_16Bit(Color);
		}
	}
	LCD_CS_SET;
} 

/*******************************************************************************
* 名    称：LCD_RESET
* 功    能：LCD复位函数
* 参数说明：无
* 返 回 值：无
* 说    明：无
*******************************************************************************/ 
static void LCD_RESET(void)
{
	LCD_RST_CLR;
	HAL_Delay(100);	
	LCD_RST_SET;
	HAL_Delay(50);
}

/*******************************************************************************
* 名    称：LCD_Init
* 功    能：LCD初始化函数
* 参数说明：无
* 返 回 值：无
* 说    明：无
*******************************************************************************/	 
void LCD_Init(void)
{  
    SPI_GPIO_Init(); 
    LCD_GPIO_init();    
 	LCD_RESET();    	
    LCD_WR_REG(0xCF);  
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0xC1); 
	LCD_WR_DATA(0X30); 
	LCD_WR_REG(0xED);  
	LCD_WR_DATA(0x64); 
	LCD_WR_DATA(0x03); 
	LCD_WR_DATA(0X12); 
	LCD_WR_DATA(0X81); 
	LCD_WR_REG(0xE8);  
	LCD_WR_DATA(0x85); 
	LCD_WR_DATA(0x10); 
	LCD_WR_DATA(0x7A); 
	LCD_WR_REG(0xCB);  
	LCD_WR_DATA(0x39); 
	LCD_WR_DATA(0x2C); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x34); 
	LCD_WR_DATA(0x02); 
	LCD_WR_REG(0xF7);  
	LCD_WR_DATA(0x20); 
	LCD_WR_REG(0xEA);  
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x00); 
	LCD_WR_REG(0xC0);    
	LCD_WR_DATA(0x1B);  
	LCD_WR_REG(0xC1);
	LCD_WR_DATA(0x01); 
	LCD_WR_REG(0xC5);    
	LCD_WR_DATA(0x30); 	
	LCD_WR_DATA(0x30); 	 
	LCD_WR_REG(0xC7);    
	LCD_WR_DATA(0XB7); 
	LCD_WR_REG(0x36);   
	LCD_WR_DATA(0x48); 
	LCD_WR_REG(0x3A);   
	LCD_WR_DATA(0x55); 
	LCD_WR_REG(0xB1);   
	LCD_WR_DATA(0x00);   
	LCD_WR_DATA(0x1A); 
	LCD_WR_REG(0xB6);    
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0xA2); 
	LCD_WR_REG(0xF2);   
	LCD_WR_DATA(0x00); 
	LCD_WR_REG(0x26);  
	LCD_WR_DATA(0x01); 
	LCD_WR_REG(0xE0);   
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x2A); 
	LCD_WR_DATA(0x28); 
	LCD_WR_DATA(0x08); 
	LCD_WR_DATA(0x0E); 
	LCD_WR_DATA(0x08); 
	LCD_WR_DATA(0x54); 
	LCD_WR_DATA(0XA9); 
	LCD_WR_DATA(0x43); 
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x00); 		 
	LCD_WR_REG(0XE1);   
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x15); 
	LCD_WR_DATA(0x17); 
	LCD_WR_DATA(0x07); 
	LCD_WR_DATA(0x11); 
	LCD_WR_DATA(0x06); 
	LCD_WR_DATA(0x2B); 
	LCD_WR_DATA(0x56); 
	LCD_WR_DATA(0x3C); 
	LCD_WR_DATA(0x05); 
	LCD_WR_DATA(0x10); 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x3F); 
	LCD_WR_DATA(0x3F); 
	LCD_WR_DATA(0x0F); 
	LCD_WR_REG(0x2B); 
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x01);
	LCD_WR_DATA(0x3f);
	LCD_WR_REG(0x2A); 
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0xef);	 
	LCD_WR_REG(0x11); 
	HAL_Delay (120);
	LCD_WR_REG(0x29); 
    LCD_direction(USE_HORIZONTAL);//设置LCD显示方向 
    LCD_LED_SET;
    LCD_Clear(WHITE);//清全屏白色
}
  
/*******************************************************************************
* 名    称：LCD_SetWindows
* 功    能：LCD设置窗口函数
* 参数说明：xStar：x轴起始点
            yStar：y轴起始点
            xEnd：x轴终止点
            yEnd：y轴中止点
* 返 回 值：无
* 说    明：无
*******************************************************************************/	
void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
{	
	LCD_WR_REG(lcddev.setxcmd);	
	LCD_WR_DATA(xStar>>8);
	LCD_WR_DATA(0x00FF&xStar);		
	LCD_WR_DATA(xEnd>>8);
	LCD_WR_DATA(0x00FF&xEnd);

	LCD_WR_REG(lcddev.setycmd);	
	LCD_WR_DATA(yStar>>8);
	LCD_WR_DATA(0x00FF&yStar);		
	LCD_WR_DATA(yEnd>>8);
	LCD_WR_DATA(0x00FF&yEnd);

	LCD_WriteRAM_Prepare();		
}   

/*******************************************************************************
* 名    称：LCD_SetCursor
* 功    能：LCD设置光标函数
* 参数说明：Xpos:x轴位置
            Ypos：y轴位置
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{	  	    			
	LCD_SetWindows(Xpos,Ypos,Xpos,Ypos);	
} 

/*******************************************************************************
* 名    称：LCD_direction
* 功    能：LCD设置方向函数
* 参数说明：direction:0  0°
                      1  90°
                      2  180°
                      3  270°
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void LCD_direction(uint8_t direction)
{ 
    lcddev.setxcmd=0x2A;
    lcddev.setycmd=0x2B;
    lcddev.wramcmd=0x2C;
	switch(direction)
    {		  
		case 0:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;		
			LCD_WriteReg(0x36,(1<<3)|(0<<6)|(0<<7));
		break;
		case 1:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(0<<7)|(1<<6)|(1<<5));
		break;
		case 2:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;	
			LCD_WriteReg(0x36,(1<<3)|(1<<6)|(1<<7));
		break;
		case 3:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(1<<7)|(1<<5));
		break;	
		default:break;
	}		
}	 

/*******************************************************************************
* 名    称：LCD_draw_bmp
* 功    能：LCD绘制图片函数
* 参数说明：x；x轴位置
            y:y轴位置
            bmp_size:图片尺寸
            buf:图片数据
* 返 回 值：无
* 说    明：无
*******************************************************************************/
void LCD_draw_bmp(uint16_t x,uint16_t y,uint8_t bmp_size,const uint8_t *buf)
{  							  
    uint16_t t1,t;
	uint16_t temp=0;
	uint8_t *p=NULL;
    
	for(t=0;t<bmp_size;t++)
	{   						
		for(t1=0;t1<bmp_size;t1++)
		{	
			p=(uint8_t*)&buf[(t*bmp_size+t1)*2];
			temp=Byte_to_Word(p);
				LCD_DrawPoint1(x+t1,y+t,temp);		
		}  	 
	}  	    	   	 	  
}

static void LCD_DMA_TxCpltCallback(DMA_HandleTypeDef *hdma)
{
	(void)hdma;
	CLEAR_BIT(hspi1.Instance->CR2, SPI_CR2_TXDMAEN);

	uint32_t guard = 20000U;
	while (((hspi1.Instance->SR & SPI_SR_FTLVL) != 0U ||
	        (hspi1.Instance->SR & SPI_SR_BSY) != 0U) && guard > 0U) {
		guard--;
	}

	LCD_CS_SET;

	__HAL_SPI_DISABLE(&hspi1);
	while ((hspi1.Instance->SR & SPI_SR_FRLVL) != 0U) {
		(void)*((__IO uint16_t *)&hspi1.Instance->DR);
	}
	__HAL_SPI_CLEAR_OVRFLAG(&hspi1);
	MODIFY_REG(hspi1.Instance->CR2, SPI_CR2_DS | SPI_CR2_FRXTH,
	           SPI_DATASIZE_8BIT | SPI_RXFIFO_THRESHOLD_QF);
	__HAL_SPI_ENABLE(&hspi1);

	lcd_dma_busy = 0U;
}

void LCD_DMA_Init(void)
{
	if (lcd_dma_inited) {
		return;
	}
	__HAL_RCC_DMA1_CLK_ENABLE();

	hdma_spi1_tx.Instance = DMA1_Channel3;
	hdma_spi1_tx.Init.Request = DMA_REQUEST_1;
	hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
	hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
	hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdma_spi1_tx.Init.Mode = DMA_NORMAL;
	hdma_spi1_tx.Init.Priority = DMA_PRIORITY_HIGH;
	HAL_DMA_Init(&hdma_spi1_tx);

	__HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);
	hdma_spi1_tx.XferCpltCallback = LCD_DMA_TxCpltCallback;
	hdma_spi1_tx.XferErrorCallback = LCD_DMA_TxCpltCallback;

	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	lcd_dma_inited = 1U;
}

void LCD_BlitRGB565_Wait(void)
{
	while (lcd_dma_busy != 0U) {
	}
}

void LCD_BlitRGB565_AsyncStart(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels)
{
	uint32_t count = (uint32_t)w * (uint32_t)h;
	if (count == 0U) {
		return;
	}
	if (!lcd_dma_inited) {
		LCD_DMA_Init();
	}
	LCD_BlitRGB565_Wait();
	LCD_SetWindows(x, y, x + w - 1U, y + h - 1U);

	__HAL_SPI_DISABLE(&hspi1);
	while ((hspi1.Instance->SR & SPI_SR_FRLVL) != 0U) {
		(void)*((__IO uint8_t *)&hspi1.Instance->DR);
	}
	__HAL_SPI_CLEAR_OVRFLAG(&hspi1);
	MODIFY_REG(hspi1.Instance->CR2, SPI_CR2_DS | SPI_CR2_FRXTH, SPI_DATASIZE_16BIT);
	__HAL_SPI_ENABLE(&hspi1);

	LCD_CS_CLR;
	LCD_RS_SET;
	lcd_dma_busy = 1U;

	if (HAL_DMA_Start_IT(&hdma_spi1_tx, (uint32_t)pixels, (uint32_t)&hspi1.Instance->DR, count) != HAL_OK) {
		lcd_dma_busy = 0U;
		LCD_CS_SET;
		__HAL_SPI_DISABLE(&hspi1);
		MODIFY_REG(hspi1.Instance->CR2, SPI_CR2_DS | SPI_CR2_FRXTH,
		           SPI_DATASIZE_8BIT | SPI_RXFIFO_THRESHOLD_QF);
		__HAL_SPI_ENABLE(&hspi1);
		return;
	}
	SET_BIT(hspi1.Instance->CR2, SPI_CR2_TXDMAEN);
}

void LCD_BlitRGB565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels)
{
	LCD_BlitRGB565_AsyncStart(x, y, w, h, pixels);
	LCD_BlitRGB565_Wait();
}

void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	uint32_t count = (uint32_t)w * (uint32_t)h;
	uint8_t hi = (uint8_t)(color >> 8);
	uint8_t lo = (uint8_t)(color & 0xFFU);
	LCD_BlitRGB565_Wait();
	LCD_SetWindows(x, y, x + w - 1U, y + h - 1U);
	LCD_CS_CLR;
	LCD_RS_SET;
	for (uint32_t i = 0; i < count; i++) {
		while ((hspi1.Instance->SR & SPI_FLAG_TXE) == 0U) {}
		*((__IO uint8_t *)&hspi1.Instance->DR) = hi;
		while ((hspi1.Instance->SR & SPI_FLAG_RXNE) == 0U) {}
		(void)*((__IO uint8_t *)&hspi1.Instance->DR);
		while ((hspi1.Instance->SR & SPI_FLAG_TXE) == 0U) {}
		*((__IO uint8_t *)&hspi1.Instance->DR) = lo;
		while ((hspi1.Instance->SR & SPI_FLAG_RXNE) == 0U) {}
		(void)*((__IO uint8_t *)&hspi1.Instance->DR);
	}
	LCD_CS_SET;
}
