/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
*/

//https://github.com/rgwan/st7565

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

#include <libopencm3/stm32/timer.h>

#include <libopencm3/cm3/systick.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <stdint.h>


#include "common.h"
#include "gui/gui.h"
#include "target/drivers/mcu/stm32/rcc.h"

#define CS_HI() GPIO_pin_set(LCD_SPI.csn)
#define CS_LO() GPIO_pin_clear(LCD_SPI.csn)
#define CMD_MODE() GPIO_pin_clear(LCD_SPI_MODE)
#define DATA_MODE() GPIO_pin_set(LCD_SPI_MODE)

#ifndef OLED_SPI_RATE
    #define OLED_SPI_RATE 0
#endif

static void LCD_Cmd(unsigned cmd) {
    /*CMD_MODE();
    CS_LO();
    spi_xfer(LCD_SPI.spi, cmd);
    CS_HI();*/

    unsigned char Num;
    gpio_clear(GPIO_CS);
    NOP();
    gpio_clear(GPIO_RS);
    NOP();
    for(Num=0;Num<8;Num++)
    {
    if((cmd&0x80) == 0) gpio_clear(GPIO_SDI);
    else gpio_set(GPIO_SDI);
    NOP();
    cmd = cmd << 1;
    gpio_clear(GPIO_SCLK);
    NOP();
    gpio_set(GPIO_SCLK);
    NOP();
    }
    NOP();
    gpio_set(GPIO_CS);
}

static void LCD_Data(unsigned cmd) {
    /*DATA_MODE();
    CS_LO();
    spi_xfer(LCD_SPI.spi, cmd);
    CS_HI();*/

    unsigned char Num;
    gpio_clear(GPIO_CS);
    NOP();
    gpio_set(GPIO_RS);
    NOP();
    for(Num=0;Num<8;Num++)
    {
    if((Dat&0x80) == 0) gpio_clear(GPIO_SDI);
    else gpio_set(GPIO_SDI);
    NOP();
    Dat = Dat << 1;
    gpio_clear(GPIO_SCLK);
    NOP();
    gpio_set(GPIO_SCLK);
    NOP();
    }
    NOP();
    gpio_set(GPIO_CS);
}

static void lcd_init_ports()
{
    // Initialization is mostly done in SPI Flash
    // Setup CS as B.0 Data/Control = C.5
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI.csn));
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI_MODE));
    GPIO_setup_output(LCD_SPI.csn, OTYPE_PUSHPULL);
    GPIO_setup_output(LCD_SPI_MODE, OTYPE_PUSHPULL);

    //GPIO
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		GPIO_CNF_OUTPUT_PUSHPULL, GPIO3 | GPIO4 |GPIO5 | GPIO6 | GPIO7 | GPIO8| GPIO9);	 

    //SET SPI BAUDRATE
    spi_set_baudrate_prescaler(LCD_SPI.spi, OLED_SPI_RATE);
}

static void Display_fill(unsigned char fill)
{
 unsigned char page,column;
 for(page=0xB7;page>=0xB0;page--)
    {
     w_cmd(page);  //set page address
     w_cmd(0x10);  //set Column address MSB
     w_cmd(0x00);  //set column address LSB
     for(column=0;column<131;column++)
        {
         w_dat(fill);
        }
    }
}


#include "128x64x1_common.h"
