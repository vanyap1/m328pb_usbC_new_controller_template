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

#include "string.h"
#include "stdbool.h"
#include "lib/uart_hal.h"
#include "lib/gpio_driver.h"
#include "lib/twi_hal1.h"
#include "lib/adc_hal.h"
#include "lib/rtc.h"
#include "stdint.h"
#include <stdio.h>
static FILE mystdout = FDEV_SETUP_STREAM((void *)uart_send_byte, NULL, _FDEV_SETUP_WRITE);


gpio rtc_int = {(uint8_t *)&PORTD , PORTD2};
gpio lcd_blk = {(uint8_t *)&PORTC , PORTC3};
gpio ext_led = {(uint8_t *)&PORTD , PORTC5};
gpio deb_led = {(uint8_t *)&PORTB , PORTB5};		
	
rtc_date sys_rtc = {.date = 18,
	.month = 10,
	.year = 23,
	.dayofweek = 1,
	.hour = 0,
	.minute = 18,
	.second = 00
};

#define LIGHT_TRASHEL_LEVEL			100
#define BAT_LOW_LEVEL				370     // =(mV/10); 370 = 3.7V
#define LAMP_HIGH_POW				255		// MAX PWM
#define LAMP_MID_POW				40
#define LAMP_OFF					0


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
	//rtc_set(&sys_rtc);
	rtc_int_enable(&sys_rtc ,0);
	adc_init();
	
	sleep_enable();
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	uart_send_string((uint8_t *)"\rReady\n\r");
    while (1) 
    {
		if (rtc_int_request != 0){
			
			rtc_sync(&sys_rtc);
			BAT_VOLT = get_mVolt(ADC5_PIN);
			rtc_int_request = 0;
			printf("Time: %02d-%02d-20%02d; %02d:%02d:%02d; BAT: %03d\r\n", sys_rtc.date, sys_rtc.month, sys_rtc.year, sys_rtc.hour, sys_rtc.minute, sys_rtc.second, BAT_VOLT);
			printf("UNIX time %u\r\n", convert_to_timestamp(&sys_rtc));
			gpio_set_pin_level(&ext_led , true);
			if(BAT_VOLT > BAT_LOW_LEVEL){
				_delay_ms(3);	
			}
			gpio_set_pin_level(&ext_led , false);
		}
		
		sleep_cpu();	
    }
}

