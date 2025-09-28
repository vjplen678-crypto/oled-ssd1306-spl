#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "stm32f10x.h"
#include <stdint.h>
#include <string.h>

#define SSD1306_WIDTH    128
#define SSD1306_HEIGHT   64
#define SSD1306_I2C_ADDR 0x78     // 0x3C<<1

typedef enum {
    Black = 0x00,
    White = 0x01
} SSD1306_COLOR;

typedef struct {
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t  Inverted;
    uint8_t  Initialized;
} SSD1306_t;


typedef struct {
    const uint8_t FontWidth;
    const uint8_t FontHeight;
} FontDef;

/* ===== API ===== */
void     ssd1306_Init(void);
void     ssd1306_UpdateScreen(void);
void     ssd1306_Fill(SSD1306_COLOR color);
void     ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void     ssd1306_SetCursor(uint8_t x, uint8_t y);
char     ssd1306_WriteChar(char ch, FontDef font, SSD1306_COLOR color);
char     ssd1306_WriteString(char* str, FontDef font, SSD1306_COLOR color);

/* I2C1 init (PB6=SCL, PB7=SDA) */
void     I2C1_Init(void);

/* font 7x18 */
extern const FontDef Font_7x18;

const uint8_t* Font7x18_GetChar(char ch);

#endif /* __SSD1306_H__ */

