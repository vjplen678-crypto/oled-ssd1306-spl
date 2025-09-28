#include "ssd1306.h"

/* ============ Buffer RAM ============ */
static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static SSD1306_t SSD1306;

/* ============ Low-level I2C helpers ============ */
static void ssd1306_WriteCommand(uint8_t cmd) {
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, SSD1306_I2C_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2C1, 0x00);            // Control byte: Co=0, D/C#=0 (command)
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_SendData(I2C1, cmd);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    I2C_GenerateSTOP(I2C1, ENABLE);
}

static void ssd1306_WriteDataBurst(const uint8_t* data, uint16_t len) {
    while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY));
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    I2C_Send7bitAddress(I2C1, SSD1306_I2C_ADDR, I2C_Direction_Transmitter);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    I2C_SendData(I2C1, 0x40);            // Control byte: Co=0, D/C#=1 (data)
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

    for (uint16_t i = 0; i < len; i++) {
        I2C_SendData(I2C1, data[i]);
        while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    }
    I2C_GenerateSTOP(I2C1, ENABLE);
}

/* ============ Public low-level ============ */
void I2C1_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef  I2C_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    /* PB6=SCL, PB7=SDA open-drain */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_DeInit(I2C1);
    I2C_InitStruct.I2C_ClockSpeed          = 400000;  // 400kHz
    I2C_InitStruct.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStruct.I2C_OwnAddress1         = 0x00;
    I2C_InitStruct.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStruct);
    I2C_Cmd(I2C1, ENABLE);
}

/* ============ SSD1306 High-level ============ */
void ssd1306_Init(void) {
    I2C1_Init();

    /* Init sequence (128x64) */
    ssd1306_WriteCommand(0xAE); // display off
    ssd1306_WriteCommand(0x20); // Memory addressing
    ssd1306_WriteCommand(0x00); // Horizontal addressing mode
    ssd1306_WriteCommand(0xB0); // page0
    ssd1306_WriteCommand(0xC8); // COM scan dir remapped
    ssd1306_WriteCommand(0x00); // low col
    ssd1306_WriteCommand(0x10); // high col
    ssd1306_WriteCommand(0x40); // start line 0
    ssd1306_WriteCommand(0x81); // contrast
    ssd1306_WriteCommand(0x7F);
    ssd1306_WriteCommand(0xA1); // segment remap
    ssd1306_WriteCommand(0xA6); // normal display
    ssd1306_WriteCommand(0xA8); // multiplex
    ssd1306_WriteCommand(0x3F);
    ssd1306_WriteCommand(0xA4); // display follows RAM
    ssd1306_WriteCommand(0xD3); // display offset
    ssd1306_WriteCommand(0x00);
    ssd1306_WriteCommand(0xD5); // clock divide
    ssd1306_WriteCommand(0x80);
    ssd1306_WriteCommand(0xD9); // pre-charge
    ssd1306_WriteCommand(0xF1);
    ssd1306_WriteCommand(0xDA); // com pins
    ssd1306_WriteCommand(0x12);
    ssd1306_WriteCommand(0xDB); // vcom detect
    ssd1306_WriteCommand(0x40);
    ssd1306_WriteCommand(0x8D); // charge pump
    ssd1306_WriteCommand(0x14);
    ssd1306_WriteCommand(0xAF); // display on

    SSD1306.CurrentX   = 0;
    SSD1306.CurrentY   = 0;
    SSD1306.Inverted   = 0;
    SSD1306.Initialized= 1;

    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void ssd1306_Fill(SSD1306_COLOR color) {
    memset(SSD1306_Buffer, (color==Black) ? 0x00 : 0xFF, sizeof(SSD1306_Buffer));
}

void ssd1306_UpdateScreen(void) {
    for (uint8_t page = 0; page < 8; page++) {
        ssd1306_WriteCommand(0xB0 + page);
        ssd1306_WriteCommand(0x00);
        ssd1306_WriteCommand(0x10);
        ssd1306_WriteDataBurst(&SSD1306_Buffer[SSD1306_WIDTH * page], SSD1306_WIDTH);
    }
}

void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color) {
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    if (SSD1306.Inverted) color = (SSD1306_COLOR)!color;

    uint32_t index = x + (y / 8) * SSD1306_WIDTH;
    if (color == White) {
        SSD1306_Buffer[index] |=  (1 << (y % 8));
    } else {
        SSD1306_Buffer[index] &= ~(1 << (y % 8));
    }
}

void ssd1306_SetCursor(uint8_t x, uint8_t y) {
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}


char ssd1306_WriteChar(char ch, FontDef font, SSD1306_COLOR color) {
    const uint8_t* glyph = Font7x18_GetChar(ch);
    if (!glyph) return 0;

  
    if (SSD1306.CurrentX + font.FontWidth  > SSD1306_WIDTH  ||
        SSD1306.CurrentY + font.FontHeight > SSD1306_HEIGHT) {
        return 0;
    }

 for (uint8_t row = 0; row < font.FontHeight; row++) {
    uint8_t bits = glyph[row];
    for (uint8_t col = 0; col < font.FontWidth; col++) {
    
        if (bits & (1u << (font.FontWidth - 1 - col))) {
            ssd1306_DrawPixel(SSD1306.CurrentX + col, SSD1306.CurrentY + row, color);
        } else {
            ssd1306_DrawPixel(SSD1306.CurrentX + col, SSD1306.CurrentY + row, (SSD1306_COLOR)!color);
        }
    }
}


    SSD1306.CurrentX += font.FontWidth + 1; // +1 kho?ng tr?ng
    return ch;
}

char ssd1306_WriteString(char* str, FontDef font, SSD1306_COLOR color) {
    while (*str) {
        if (ssd1306_WriteChar(*str, font, color) != *str) return *str;
        str++;
    }
    return 0;
}

