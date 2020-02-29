#ifndef HAS_LCD_FLIPPED
    #define HAS_LCD_FLIPPED 0
#endif
#ifndef HAS_LCD_SWAPPED_PAGES
    #define HAS_LCD_SWAPPED_PAGES 0
#endif

#include "128x64x1_ST7565.h"

//The screen is 129 characters, but we'll only expoise 128 of them
#define PHY_LCD_WIDTH 129
#define LCD_PAGES 8
static u8 img[PHY_LCD_WIDTH * LCD_PAGES];
static u8 dirty[PHY_LCD_WIDTH];
static u16 xstart, xend;  // After introducing logical view for devo10, the coordinate can be >= 5000
static u16 xpos, ypos;
static s8 dir;

void lcd_display(uint8_t on)
{
    LCD_Cmd(0xAE | (on ? 1 : 0));

    unsigned char page,column;
 for(page=0xB0;page<=0xB7;page++)
    {
     w_cmd(page);  //set page address
     w_cmd(0x10);  //set Column address MSB
     w_cmd(0x00);  //set column address LSB
     for(column=0;column<128;column++)
        {
         w_dat(*p++);
        }
    }
}

void lcd_set_page_address(uint8_t page)
{
    LCD_Cmd(0xB0 | (page & 0x07));
}

void lcd_set_column_address(uint8_t column)
{
    LCD_Cmd(0x10 | ((column >> 4) & 0x0F));  //MSB
    LCD_Cmd(column & 0x0F);                  //LSB
}

void lcd_set_start_line(int line)
{
  LCD_Cmd((line & 0x3F) | 0x40);
}

void LCD_Contrast(unsigned contrast)
{
    //int data = 0x20 + contrast * 0xC / 10;
    LCD_Cmd(0x81);
    if (HAS_OLED_DISPLAY) {
        contrast = _oled_contrast_func(contrast);
    } else {
        contrast = _lcd_contrast_func(contrast);
    }
    LCD_Cmd(contrast);
}

void LCD_Init()
{
    /*lcd_init_ports();

    _lcd_reset();

    _lcd_init();

    volatile int i = 0x8000;
    while(i) i--;
    lcd_display(0);     // Display Off
    lcd_set_start_line(0);
    // Display data write (6)
    // Clear the screen
    for(int page = 0; page < LCD_PAGES; page++) {
        lcd_set_page_address(page);
        lcd_set_column_address(0);
        for(int col = 0; col < PHY_LCD_WIDTH; col++)
            LCD_Data(0x00);
    }
    lcd_display(1);
    LCD_Contrast(5);
    memset(img, 0, sizeof(img));
    memset(dirty, 0, sizeof(dirty));*/


    gpio_clear(GPIO_CS);
    LCD_Reset();
    w_cmd(0xA3);    //LCD Bias	 selection(1/65 Duty,1/7Bias)
    w_cmd(0xA0);    //ADC selection(SEG0->SEG128)
    w_cmd(0xC8);    //SHL selection(COM0->COM64)

    w_cmd(0x26);    //Regulator Resistor Selection
    delay_ms(5);
    w_cmd(0x81);    //Electronic Volume
    w_cmd(0x20);    //Reference Register selection  Vo=(1+Rb/Ra)(1+a)*2.1=10
    w_cmd(VC_ON);    //Power Control(Vc=1;Vr=0;Vf=0)
    delay_ms(10);
    w_cmd(VC_ON|VR_ON);
    delay_ms(10);
    w_cmd(VC_ON|VR_ON|VF_ON);
    delay_ms(10);
    w_cmd(0xF8);
    w_cmd(0x01);

    delay_ms(5);
    w_cmd(0xAF);    //Display on
    w_cmd(0xA6);


}

void LCD_Clear(unsigned int val)
{
    val = (val & 0xFF) ? 0xff : 0x00;
    memset(img, val, sizeof(img));
    memset(dirty, 0xFF, sizeof(dirty));
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir _dir)
{
    if (_dir == DRAW_SWNE) {
        ypos = y1;  // bug fix: must do it this way to draw bmp
        dir = -1;
    } else {
        ypos = y0;
        dir = 1;
    }
    xstart = x0;
    xend = x1;
    xpos = x0;
}
/* Screen coordinates are as follows:
 * (128, 32)   ....   (0, 32)
 *   ...       ....     ...
 * (128, 63)   ....   (0, 63)
 * (128, 0)    ....   (0, 0)
 *   ...       ....     ...
 * (128, 31)   ....   (0, 31)
 */
void LCD_DrawStop(void)
{
    int col = 0;
    int p, c;
    for (p = 0; p < LCD_PAGES; p++) {
        int init = 0;
        for (c = 0; c < PHY_LCD_WIDTH; c++) {
            if(dirty[c] & (1 << p)) {
                if(! init) {
                    lcd_set_page_address(p);
                    lcd_set_column_address(c);
                } else if(col+1 != c) {
                    lcd_set_column_address(c);
                }
                LCD_Data(img[p * PHY_LCD_WIDTH + c]);
                col = c;
            }
        }
    }
    memset(dirty, 0, sizeof(dirty));
}

/*
 * 2.
Display Start Line Set
Specifies line address (refer to Figure 6) to determine the initial display line, or COM0. The RAM
display data becomes the top line of LCD screen. The higher number of
lines in ascending order,
corresponding to the duty cycle follows it. When this command changes the line address, smooth
scrolling or a page change takes place.
A0 (E /RD) (R/W /WR) D7 D6 D5 D4 D3 D2 D1 D0 Hex
0     1        0      0  1 A5 A4 A3 A2 A1 A0 40h to 7Fh

8.
ADC Select
Changes the relationship between RAM column address and segment driver. The order of
segment driver output pads could be reversed by software. This allows flexi
ble IC layout during
LCD module assembly. For details, refer to the column address section of Figure 4. When display
data is written or read, the column address is incremented by 1 as shown in Figure 4.
A0 (E /RD) (R/W /WR) D7 D6 D5 D4 D3 D2 D1 D0 Hex Setting
 0    1        0      1  0  1  0  0  0  0  0 A0h Normal
                                           1 A1h Reverse
 */
void LCD_DrawPixel(unsigned int color)
{
    if (xpos < LCD_WIDTH && ypos < LCD_HEIGHT) {    // both are unsigned, can not be < 0
        int y = ypos;
        int x = xpos;
        if (HAS_LCD_SWAPPED_PAGES) {
            x = PHY_LCD_WIDTH - 1 - xpos;  // We want to map 0 -> 128 and 128 -> 0
            if (ypos > 31)
                y = ypos - 32;
            else
                y = ypos + 32;
        }
        int ycol = y / 8;
        int ybit = y & 0x07;
        if (color) {
            img[ycol * PHY_LCD_WIDTH + x] |= 1 << ybit;
        } else {
            img[ycol * PHY_LCD_WIDTH + x] &= ~(1 << ybit);
        }
        dirty[x] |= 1 << ycol;
    }
    // this must be executed to continue drawing in the next row
    xpos++;
    if (xpos > xend) {
        xpos = xstart;
        ypos += dir;
    }
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    xpos = x;
    ypos = y;
    LCD_DrawPixel(color);
}
