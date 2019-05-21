/*
 * global_var.h
 *
 *  Created on: May 10, 2019
 *      Author: alb3rto
 */

#ifndef MAIN_GLOBAL_H_
#define MAIN_GLOBAL_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


SemaphoreHandle_t sem_timer;
#define MAX_UI16		65536


#endif /* MAIN_GLOBAL_H_ */
