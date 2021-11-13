#include "can_message_utils.h"

static void can_listener_task(void *arg) {
    xSemaphoreTake(rx_sem, portMAX_DELAY);
    twai_status_info_t status_info;
    uint64_t data;
    uint32_t msgs_to_rx = 0;
    while (true) {
        ESP_ERROR_CHECK(twai_get_status_info(&status_info));
        msgs_to_rx = status_info.msgs_to_rx;
        while (msgs_to_rx > 0) {
            twai_message_t rx_msg;
            ESP_ERROR_CHECK(twai_receive(&rx_msg, portMAX_DELAY));
            data = read_data(&rx_msg);
            print_message(rx_msg.identifier, rx_msg.data_length_code, data);
            msgs_to_rx--;
        }
        vTaskDelay(1);
    }
    xSemaphoreGive(rx_sem);
    vTaskDelete(NULL);
}
