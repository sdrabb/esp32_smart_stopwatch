#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "u8g2_esp32_hal.h"
#include "hc_sr04.h"
#include "ssd1306.h"
#include "timer.h"
#include "global.h"


void app_main(void){

    nvs_flash_init();

    // init sempahore
    sem_timer = xSemaphoreCreateBinary();

    xTaskCreate(task_hcsc04, "task_hcsc04", 2048, NULL, 1, NULL );
    xTaskCreate(task_ssd1306, "task_ssd1306", 2048, NULL, 1, NULL );
    //xTaskCreate(task_timer, "task_timer", 2048, NULL, 1, NULL );
}

