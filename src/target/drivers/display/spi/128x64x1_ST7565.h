#define LCD_SPI_RATE SPI_CR1_BR_FPCLK_DIV_4
#define OLED_SPI_RATE SPI_CR1_BR_FPCLK_DIV_8

void _lcd_reset()
{
 gpio_clear(GPIO_RES);
 delay_ms(50);
 gpio_set(GPIO_RES);
 delay_ms(50);
}