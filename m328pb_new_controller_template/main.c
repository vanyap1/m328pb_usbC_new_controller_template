/*
 * m328pb_new_controller_template.c
 *
 * Created: 17.10.2023 23:10:12
 * Author : Vanya
 */ 


#include "config.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>


#include "display_fn.h"
#include "u8g2.h"
#include "mui.h"
#include "mui_u8g2.h"
#include "string.h"
#include "stdbool.h"
#include "uart_hal.h"
#include "gpio_driver.h"
#include "twi_hal1.h"
#include "adc_hal.h"
#include "spi1_hall.h"
#include "rtc.h"
#include "stdint.h"
#include "bms_ina22x.h"
#include <stdio.h>
static FILE mystdout = FDEV_SETUP_STREAM((void *)uart_send_byte, NULL, _FDEV_SETUP_WRITE);

u8g2_t lcd;
uint8_t display_line[64];

gpio rtc_int = {(uint8_t *)&PORTD , PORTD2};
gpio lcd_blk = {(uint8_t *)&PORTC , PORTC3};
gpio ext_led = {(uint8_t *)&PORTD , PORTC5};
gpio deb_led = {(uint8_t *)&PORTB , PORTB5};		
	
rtc_date sys_rtc = {
	.date = 27,
	.month = 10,
	.year = 23,
	.dayofweek = 6,
	.hour = 16,
	.minute = 52,
	.second = 00
};

powerData dc_source1;


#define LIGHT_TRASHEL_LEVEL			100
#define BAT_LOW_LEVEL				370     // =(mV/10); 370 = 3.7V
#define LAMP_HIGH_POW				255		// MAX PWM
#define LAMP_MID_POW				40
#define LAMP_OFF					0
#define LCD_CONTRAST				30

#define  INA226ADR					0x40

uint8_t rtc_int_request = 0;
uint16_t BAT_VOLT = 0;




ISR(INT0_vect){
	rtc_int_request=1;
}



int main(void)
{
    char char_array[128]="\0";

    
    uart_init(250000,1);
    twi_init(400000);
    gpio_set_pin_direction(&lcd_blk , PORT_DIR_OUT); gpio_set_pin_level(&lcd_blk, false);
	gpio_set_pin_direction(&ext_led , PORT_DIR_OUT); gpio_set_pin_level(&ext_led, false);
	gpio_set_pin_direction(&deb_led , PORT_DIR_OUT); gpio_set_pin_level(&deb_led, false);
    gpio_set_pin_direction(&rtc_int, PORT_DIR_IN);
    EICRA |= (0b10 << ISC00);
    EIMSK = 0x01; //0b00000001
	
	sei();
	stdout = &mystdout;
	rtc_set(&sys_rtc);
	rtc_int_enable(&sys_rtc ,0);
	adc_init();
	spi1_init();
	lcd_gpio_init();
	
	
	
	//u8g2_Setup_st7565_zolen_128x64_f( &lcd, U8G2_MIRROR_VERTICAL, lcd_hw_spi, fake_delay_fn);					//HW SPI
	u8g2_Setup_ssd1306_i2c_128x32_univision_f(&lcd, U8G2_R0, u8x8_byte_sw_i2c, fake_delay_fn);					//0.91 OLED 128x32
	//u8g2_Setup_st7565_zolen_128x64_f(&lcd, U8G2_MIRROR_VERTICAL, u8x8_byte_4wire_sw_spi, lcd_software_spi);	//SW SPI
	
	
	//OLED display
	
	//u8x8_byte_stm32hal_hw_i2c
	u8g2_SetI2CAddress(&lcd, 0x3C);

	
	//End of OLED Display
	

	
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	PowerMeterInit(INA226ADR);
	
	uart_send_string((uint8_t *)"\rReady\n\r");
	u8g2_InitDisplay(&lcd);
	u8g2_SetPowerSave(&lcd, 0);
	//u8g2_SetContrast(&lcd, LCD_CONTRAST);
	u8g2_SetFlipMode(&lcd, 1);
	u8g2_SetContrast(&lcd, 120);
	u8g2_ClearBuffer(&lcd);
	//u8g2_SetFont(&lcd, u8g2_font_5x8_t_cyrillic);
	u8g2_SetFont(&lcd, u8g2_font_6x10_mf);
	//u8g2_SetFont(&lcd, u8g2_font_ncenB14_tr);
	u8g2_DrawStr(&lcd, 1, 10, (void *)"RX MODULE");
	u8g2_SendBuffer(&lcd);
    while (1) 
    {
		if (rtc_int_request != 0){
			
			rtc_sync(&sys_rtc);
			BAT_VOLT = get_mVolt(ADC5_PIN);
			rtc_int_request = 0;
			sprintf(char_array, "Time: %02d-%02d-20%02d; %02d:%02d:%02d; BAT: %03d\r\n", sys_rtc.date, sys_rtc.month, sys_rtc.year, sys_rtc.hour, sys_rtc.minute, sys_rtc.second, BAT_VOLT);
			sprintf(char_array, "Date: %02d-%02d-20%02d", sys_rtc.date, sys_rtc.month, sys_rtc.year);
			u8g2_ClearBuffer(&lcd);
			u8g2_SetFont(&lcd, u8g2_font_6x10_mf);
			u8g2_DrawStr(&lcd, 1, 8, (void *)char_array);
			
			sprintf(char_array, "Time: %02d:%02d:%02d", sys_rtc.hour, sys_rtc.minute, sys_rtc.second);
			u8g2_DrawStr(&lcd, 1, 17, (void *)char_array);
			//
			sprintf(char_array, "BAT: %03d", BAT_VOLT);
			u8g2_DrawStr(&lcd, 1, 25, (void *)char_array);
			//
			//uint32_t UNIXtime = convert_to_timestamp(&sys_rtc);
			//sprintf(char_array, "UNIX time %lu", UNIXtime);
			//u8g2_DrawStr(&lcd, 1, 32, (void *)char_array);	
			//printf("UNIX time %lu\r\n", UNIXtime);
			
			
			PowerMeterMeasure(&dc_source1);
			
			sprintf(char_array, "mV %05u  ", dc_source1.voltage);
			u8g2_DrawStr(&lcd, 1, 34, (void *)char_array);
			
			sprintf(char_array, "mA %05d  ", dc_source1.current);
			u8g2_DrawStr(&lcd, 1, 42, (void *)char_array);
			
			sprintf(char_array, "W  %3.5f  ", dc_source1.power);
			u8g2_DrawStr(&lcd, 1, 50, (void *)char_array);
				
			sprintf(char_array, "Wh %3.5f  ", dc_source1.energy);
			u8g2_DrawStr(&lcd, 1, 59, (void *)char_array);
			
			//for (int i = 0; i <= 255; ++i) {
				//uint8_t det = PowerMeterInit(i);
				//if (det == 0){
					//sprintf(char_array, "PM_ID %03d; %03d;   ", det , i);
					//u8g2_DrawStr(&lcd, 1, 34, (void *)char_array);
					//u8g2_SendBuffer(&lcd);
					//_delay_ms(500);
				//}
				//
				
				
			//}
			
			//u8g2_DrawButtonUTF8(&lcd, 100, 30, 1, 15, 1, 1 , "OK");
			//u8g2_DrawTriangle(&lcd, 100, 32, 110, 21, 124, 32);
			
			
			
			
			
			//if(BAT_VOLT < BAT_LOW_LEVEL){
				//_delay_ms(3);	
			//}
			//gpio_set_pin_level(&ext_led , true);
			u8g2_SendBuffer(&lcd);
			//gpio_set_pin_level(&ext_led , false);
			
		}
		
		sleep_cpu();	
    }
}

