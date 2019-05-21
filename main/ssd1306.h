/*
 * ssd1306.h
 *
 *  Created on: May 8, 2019
 *      Author: alb3rto
 */

#ifndef MAIN_SSD1306_H_
#define MAIN_SSD1306_H_

enum ssd1306_states{IDLE,TIMING,SHOWING_TIME_TABLE,METER_COMPUTATION};

uint8_t ssd1306_get_opmode();
void ssd1306_set_opmode_idle();
void ssd1306_set_opmode_running();
void ssd1306_set_opmode_meter_computation();

void task_ssd1306(void *pvParameter);

#endif /* MAIN_SSD1306_H_ */
