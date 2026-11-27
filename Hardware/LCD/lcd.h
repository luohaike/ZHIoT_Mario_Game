#ifndef __LCD_H
#define __LCD_H		
#include "stm32l4xx_hal.h" 
#include "stdlib.h"

/*****************************用户配置区***************************************/	
#define USE_HORIZONTAL   0   //定义液晶屏顺时针旋转方向 0-0度旋转，1-90度旋转,2-180度旋转，3-270度旋转

/*屏幕尺寸*/
#define LCD_W           240
#define LCD_H           320  

/*画笔颜色*/
#define WHITE       0xFFFF
#define BLACK      	0x0000	  
#define BLUE       	0x001F  
#define BRED        0XF81F
#define GRED 		0XFFE0
#define GBLUE		0X07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define GREEN       0x07E0
#define CYAN        0x7FFF
#define YELLOW      0xFFE0
#define BROWN 		0XBC40 //棕色
#define BRRED 		0XFC07 //棕红色
#define GRAY  		0X8430 //灰色

/*GUI颜色*/
#define DARKBLUE    0X01CF //深蓝色
#define LIGHTBLUE   0X7D7C //浅蓝色  
#define GRAYBLUE    0X5458 //灰蓝色

/*以上三色为PANEL的颜色*/
#define LIGHTGREEN  0X841F //浅绿色
#define LIGHTGRAY   0XEF5B //浅灰色(PANNEL)
#define LGRAY 		0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE   0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE      0X2B12 //浅棕蓝色(选择条目的反色)

/**************************结构体变量声明**************************************/	
typedef struct  
{										    
    uint16_t width;		//LCD 宽度
    uint16_t height;	//LCD 高度
    uint16_t id;		//LCD ID
    uint8_t  dir;		//横屏还是竖屏控制：0，竖屏；1，横屏。	
    uint16_t wramcmd;	//开始写gram指令
    uint16_t setxcmd;	//设置x坐标指令
    uint16_t setycmd;	//设置y坐标指令	 
}_lcd_dev; 	
 
extern _lcd_dev  lcddev;	    //管理LCD重要参数
extern uint16_t  POINT_COLOR ;  //默认红色    
extern uint16_t  BACK_COLOR ;   //背景颜色.默认为白色

/*****************************外部函数声明*************************************/	    															  
void Lcd_WriteData_16Bit(uint16_t Data); 
void LCD_DrawPoint(uint16_t x,uint16_t y);
void LCD_Clear(uint16_t Color);
void LCD_Init(void);
void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd);
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos);
void LCD_direction(uint8_t direction );
void LCD_draw_bmp(uint16_t x,uint16_t y,uint8_t bmp_size,const uint8_t *buf);
void LCD_DMA_Init(void);
void LCD_BlitRGB565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels);
void LCD_BlitRGB565_AsyncStart(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *pixels);
void LCD_BlitRGB565_Wait(void);
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

#endif  
	
