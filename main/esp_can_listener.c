/* TWAI Network Listen Only Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * The following example demonstrates a Listen Only node in a TWAI network. The
 * Listen Only node will not take part in any TWAI bus activity (no acknowledgments
 * and no error frames). This example will execute multiple iterations, with each
 * iteration the Listen Only node will do the following:
 * 1) Listen for ping and ping response
 * 2) Listen for start command
 * 3) Listen for data messages
 * 4) Listen for stop and stop response
 */
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

/* --------------------- Definitions and static variables ------------------ */
//Example Configuration
#define RX_TASK_PRIO                    9
#define TX_GPIO_NUM                     CONFIG_EXAMPLE_TX_GPIO_NUM
#define RX_GPIO_NUM                     CONFIG_EXAMPLE_RX_GPIO_NUM
#define EXAMPLE_TAG                     "TWAI Listen Only"

#define ID_ENGINE_COOLANT_TEMPERATURE   0x005
#define ID_VEHICLE_SPEED                0x00D
#define ID_ENGINE_OIL_TEMPERATURE       0x05C
#define ID_TRANSMISSION_GEAR            0x0A4

static const twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
//Set TX queue length to 0 due to listen only mode
static const twai_general_config_t g_config = {.mode = TWAI_MODE_LISTEN_ONLY,
                                              .tx_io = TX_GPIO_NUM, .rx_io = RX_GPIO_NUM,
                                              .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,
                                              .tx_queue_len = 0, .rx_queue_len = 5,
                                              .alerts_enabled = TWAI_ALERT_NONE,
                                              .clkout_divider = 0};

static SemaphoreHandle_t rx_sem;

/* --------------------------- Tasks and Functions -------------------------- */

static uint32_t twai_read_data(twai_message_t * rx_msg) {
    uint32_t data = 0;
    for (int i = 0; i < rx_msg->data_length_code; i++) {
        data |= (rx_msg->data[i] << (i * 8));
    }
    return data;
}

static void twai_receive_task(void *arg)
{
    xSemaphoreTake(rx_sem, portMAX_DELAY);

    while (true) {
        twai_message_t rx_msg;
        twai_receive(&rx_msg, portMAX_DELAY);
        switch(rx_msg.identifier) {
            case ID_ENGINE_COOLANT_TEMPERATURE:
                ESP_LOGI(EXAMPLE_TAG, "ENGINE_COOLANT_TEMPERATURE %04x", twai_read_data(&rx_msg) - 40);
            break;
            case ID_VEHICLE_SPEED:
                ESP_LOGI(EXAMPLE_TAG, "VEHICLE_SPEED %04x", twai_read_data(&rx_msg));
            break;
            case ID_ENGINE_OIL_TEMPERATURE:
                ESP_LOGI(EXAMPLE_TAG, "OIL_TEMPERATURE %04x", twai_read_data(&rx_msg) - 40);
            break;
            case ID_TRANSMISSION_GEAR:
                ESP_LOGI(EXAMPLE_TAG, "TRANSMISSION_GEAR %04x", twai_read_data(&rx_msg));
            break;
        }
    }

    xSemaphoreGive(rx_sem);
    vTaskDelete(NULL);
}

void app_main(void)
{
    rx_sem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(twai_receive_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

    //Install and start TWAI driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(EXAMPLE_TAG, "Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(EXAMPLE_TAG, "Driver started");

    xSemaphoreGive(rx_sem);                     //Start RX task
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreTake(rx_sem, portMAX_DELAY);      //Wait for RX task to complete

    //Stop and uninstall TWAI driver
    ESP_ERROR_CHECK(twai_stop());
    ESP_LOGI(EXAMPLE_TAG, "Driver stopped");
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(EXAMPLE_TAG, "Driver uninstalled");

    //Cleanup
    vSemaphoreDelete(rx_sem);
}
