#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include <sys/time.h>
#include "driver/gpio.h"
#include "math.h"

#include "hc_sr04.h"
#include "ssd1306.h"
#include "global.h"

#define TAG                 				"hc_sr04 thread"
#define RMT_TX_CHANNEL      				RMT_CHANNEL_0
#define RMT_RX_CHANNEL      				RMT_CHANNEL_1
#define PING_TX_PIN         				19
#define PING_RX_PIN         				18
#define ECHO_TIMEOUT_US     				18500 //  echo timeout in μs
#define RMT_CLK_DIV         				80    // 1μs precision (80MHz/80 -> 1μs ticks)
#define RMT_TICK_10_US 						(80000000/RMT_CLK_DIV/100000) /* RMT counter value for 10 us.(Source clock is APB clock) */
#define ITEM_DURATION(d) 					((d & 0x7fff)*10/RMT_TICK_10_US)

#define CM_THRESHOLD_0						10
#define CM_THRESHOLD_1						50
#define MIN_MSG_TO_STOP						40
#define MIN_MSG_TO_START_METER_MEASURE		40


static RingbufHandle_t rx_ring_buffer_handle = NULL;

static rmt_item32_t items[] = {
    // 10μs pulse
    {{{ 10, 1, 0, 0 }}}
};

static size_t rx_size;
static float distance;

void rmt_tx_init()
{
    rmt_config_t txConfig = {
        .rmt_mode = RMT_MODE_TX,
        .channel = RMT_TX_CHANNEL,
        .gpio_num = PING_TX_PIN,
        .mem_block_num = 1,
        .tx_config.loop_en = 0,
        .tx_config.carrier_en = 0,
        .tx_config.idle_output_en = 1,
        .tx_config.idle_level = 0,
        .clk_div = RMT_CLK_DIV
    };
    ESP_ERROR_CHECK(rmt_config(&txConfig));
    ESP_ERROR_CHECK(rmt_driver_install(txConfig.channel, 0, 0));
}

void rmt_rx_init()
{
    rmt_config_t rxConfig = {
        .rmt_mode = RMT_MODE_RX,
        .channel = RMT_RX_CHANNEL,
        .gpio_num = PING_RX_PIN,
        .clk_div = RMT_CLK_DIV,
        .mem_block_num = 1,
        .rx_config.idle_threshold = ECHO_TIMEOUT_US
    };
    ESP_ERROR_CHECK(rmt_config(&rxConfig));
    ESP_ERROR_CHECK(rmt_driver_install(rxConfig.channel, 128, 0));
    ESP_ERROR_CHECK(rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rx_ring_buffer_handle));
}

float hcsr04_get_current_distance(){
	return distance;
}

void task_hcsc04(void *pvParameter)
{
    uint16_t msg_count = 0;
    uint16_t count_since_enabled = 0;
    uint16_t count_since_disabled = 0;

    // init rmt tx and rx
    rmt_tx_init();
    rmt_rx_init();

    while(1){
        // Write the 10us trigger pulse
        ESP_ERROR_CHECK(rmt_write_items(RMT_TX_CHANNEL, items, 1, true));
        ESP_ERROR_CHECK(rmt_rx_start(RMT_RX_CHANNEL, 1));

        rx_size = 0;
        rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rx_ring_buffer_handle, &rx_size, 1000);

        if (item == NULL){
            ESP_LOGW(TAG, "RMT read timeout");
        }
        else if(rx_size){
        	// got correct data in size
        	// distance in cm
            distance = 100 * 340.29 * ITEM_DURATION(item->duration0) / (1000 * 1000 * 2); // distance in meters
            vRingbufferReturnItem(rx_ring_buffer_handle, (void*) item);
            ESP_LOGI(TAG, "distance cm: %f\n",distance);

            // increment msg count
            msg_count = msg_count+1%MAX_UI16;

            // if distance is lower than CM_THRESHOLD start counting
            if((CM_THRESHOLD_0<distance&&distance<CM_THRESHOLD_1) && ssd1306_get_opmode()==IDLE && abs(msg_count-count_since_disabled)>MIN_MSG_TO_STOP){
            	ssd1306_set_opmode_running();
            	count_since_enabled = msg_count;
            }
            // stop timing
            else if((CM_THRESHOLD_0<distance&&distance<CM_THRESHOLD_1) && (ssd1306_get_opmode()==TIMING||ssd1306_get_opmode()==METER_COMPUTATION) && abs(msg_count-count_since_enabled)>MIN_MSG_TO_STOP){
            	ssd1306_set_opmode_idle();
            	count_since_disabled = msg_count;
            }
            // start meter computation
            if(distance<CM_THRESHOLD_0 && ssd1306_get_opmode()==IDLE && abs(msg_count-count_since_disabled)>MIN_MSG_TO_START_METER_MEASURE){
				ssd1306_set_opmode_meter_computation();
				count_since_enabled = msg_count;
			}

        }
        else{
        	// size does not match
            ESP_LOGI(TAG, "Received end packet");
            vRingbufferReturnItem(rx_ring_buffer_handle, (void*) item);
        }
    }
}
