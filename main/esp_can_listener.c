#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

/* --------------------- Notes ------------------ */
// mode 1 requests
// Wikipedia
// #define ID_ISO_VEHICLE_SPEED            0x0D
// #define ID_ISO_TRANSMISSION_GEAR        0xA4
// http://autowp.github.io/
// #define ID_DASHBOARD                    0x0F6

/* --------------------- Definitions and static variables ------------------ */
//Example Configuration
#define RX_TASK_PRIO                    9
#define TX_GPIO_NUM                     CONFIG_EXAMPLE_TX_GPIO_NUM
#define RX_GPIO_NUM                     CONFIG_EXAMPLE_RX_GPIO_NUM
#define MAIN_TAG                        "CAN Listener"
#define CAN_TAG                         "CAN"

/* --------------------- TWAI driver configuration ------------------ */
// 0x7ff -> all;
#define PIDS_OR                         (0x349 | 0x3c9)
// 0x000 -> all;
#define PIDS_AND                        (0x349 & 0x3c9)

static const twai_filter_config_t f_config = {
    .acceptance_code = PIDS_AND << 21,
    .acceptance_mask = ((PIDS_AND ^ PIDS_OR) << 21) | 0x001FFFFF,
    .single_filter = true
};
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_general_config_t g_config = {
    .mode = TWAI_MODE_LISTEN_ONLY,
    .tx_io = TX_GPIO_NUM,
    .rx_io = RX_GPIO_NUM,
    .clkout_io = TWAI_IO_UNUSED,
    .bus_off_io = TWAI_IO_UNUSED,
    .tx_queue_len = 0, // Set TX queue length to 0 due to listen only mode
    .rx_queue_len = 512,
    .alerts_enabled = TWAI_ALERT_NONE,
    .clkout_divider = 0,
    .intr_flags = ESP_INTR_FLAG_IRAM
};

/* --------------------------- Tasks and Functions -------------------------- */
static SemaphoreHandle_t rx_sem;

// #include "esp_can_dump.h"
// #include "esp_can_bit_finder.h"
#include "esp_can_bit_watcher.h"

void app_main(void)
{
    rx_sem = xSemaphoreCreateBinary();
    xTaskCreatePinnedToCore(can_listener_task, "TWAI_rx", 4096, NULL, RX_TASK_PRIO, NULL, tskNO_AFFINITY);

    //Install and start TWAI driver
    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(MAIN_TAG, "Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(MAIN_TAG, "Driver started");

    xSemaphoreGive(rx_sem);                     //Start RX task
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreTake(rx_sem, portMAX_DELAY);      //Wait for RX task to complete

    //Stop and uninstall TWAI driver
    ESP_ERROR_CHECK(twai_stop());
    ESP_LOGI(MAIN_TAG, "Driver stopped");
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(MAIN_TAG, "Driver uninstalled");

    //Cleanup
    vSemaphoreDelete(rx_sem);
}
