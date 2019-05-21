/*
 * ssd1306.c
 *
 *  Created on: May 8, 2019
 *      Author: alb3rto
 */
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "time.h"

#include "u8g2_esp32_hal.h"

#include "ssd1306.h"
#include "global.h"
#include "hc_sr04.h"

#define	PIN_SDA			5
#define PIN_SCL			4
#define I2C_OLED_ADDR	0x78


static uint8_t ssd1306_opmode;

uint8_t ssd1306_get_opmode(){
	return ssd1306_opmode;
}

void ssd1306_set_opmode_idle(){
	ssd1306_opmode = IDLE;
}

void ssd1306_set_opmode_running(){
	ssd1306_opmode = TIMING;
}

void ssd1306_set_opmode_meter_computation(){
	ssd1306_opmode = METER_COMPUTATION;
}

void task_ssd1306(void *pvParameter){
	char str_to_print[5];

	clock_t start_time = 0;
	clock_t current_time = 0;
	float current_time_sec = 0;

	// initialize the u8g2 hal
    u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
    u8g2_esp32_hal.sda = PIN_SDA;
    u8g2_esp32_hal.scl = PIN_SCL;
    u8g2_esp32_hal_init(u8g2_esp32_hal);
	// initialize the u8g2 library
    u8g2_t u8g2;
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0,u8g2_esp32_i2c_byte_cb,u8g2_esp32_gpio_and_delay_cb);
	// set the display address
    u8x8_SetI2CAddress(&u8g2.u8x8,I2C_OLED_ADDR);
	// initialize the display
    u8g2_InitDisplay(&u8g2);
	// wake up the display
    u8g2_SetPowerSave(&u8g2, 0);

    // set opmode to IDLE
    ssd1306_set_opmode_idle();

	// loop
	while(1) {

		switch(ssd1306_opmode){

			case IDLE:
				sprintf(str_to_print,"%.2f sec",current_time_sec);
				// set font and write last time
				u8g2_ClearBuffer(&u8g2);
				u8g2_SetFont(&u8g2, u8g2_font_timR14_tf);
				u8g2_DrawStr(&u8g2, 2,17,"Waiting...");
				u8g2_DrawStr(&u8g2, 2,34,"last time");
				u8g2_DrawStr(&u8g2, 2,51,str_to_print);
				u8g2_SendBuffer(&u8g2);
				// get current cpu time
				start_time = clock();
				break;

			case TIMING:
				// current time in ms
				current_time = (clock() - start_time)/portTICK_PERIOD_MS;
				current_time_sec = (float)current_time/100;
				// build oled second line
				sprintf(str_to_print,"%.2f sec",current_time_sec);
				// set font and write hello world
				u8g2_ClearBuffer(&u8g2);
				u8g2_SetFont(&u8g2, u8g2_font_timR14_tf);
				u8g2_DrawStr(&u8g2, 2,17,"lap time:");
				u8g2_DrawStr(&u8g2, 2,34,str_to_print);
				u8g2_SendBuffer(&u8g2);
				break;

			case METER_COMPUTATION:
				u8g2_ClearBuffer(&u8g2);
				u8g2_SetFont(&u8g2, u8g2_font_timR14_tf);
				u8g2_DrawStr(&u8g2, 2,17,"distance in cm:");
				sprintf(str_to_print,"%.2f",hcsr04_get_current_distance());
				u8g2_DrawStr(&u8g2, 2,34,str_to_print);
				u8g2_SendBuffer(&u8g2);
				break;

		}

	}

}


