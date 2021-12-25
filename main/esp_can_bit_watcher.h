#include "can_message_utils.h"

typedef struct {
    uint8_t data_length_code;
    uint64_t data;
} can_message_value_t;

static can_message_value_t can_message_values_by_id[0x7ff];

static void log_can_event(
        twai_message_t * rx_msg,
        uint64_t value_mask
    ) {
    can_message_value_t can_message_value = can_message_values_by_id[rx_msg->identifier];
    uint64_t data = read_data(rx_msg);
    if (can_message_value.data_length_code == 0) {
        can_message_value.data_length_code = rx_msg->data_length_code;
        can_message_value.data = data;
        print_message(
            rx_msg->identifier,
            rx_msg->data_length_code,
            can_message_value.data
        );
        can_message_values_by_id[rx_msg->identifier] = can_message_value;
    } else {
        uint64_t last_data = can_message_value.data;
        uint64_t diff_data = (data & value_mask) ^ (last_data & value_mask);
        if(diff_data != 0) {
            can_message_value.data_length_code = rx_msg->data_length_code;
            can_message_value.data = data;
            print_messages_diff(
                rx_msg->identifier,
                rx_msg->data_length_code,
                last_data,
                can_message_value.data,
                diff_data
            );
            can_message_values_by_id[rx_msg->identifier] = can_message_value;
        }
    }
}

static void can_listener_task(void *arg)
{
    // Wait for the initialization
    xSemaphoreTake(rx_sem, portMAX_DELAY);
    twai_status_info_t status_info;
    uint32_t msgs_to_rx = 0;
    while (true) {
        ESP_ERROR_CHECK(twai_get_status_info(&status_info));
        msgs_to_rx = status_info.msgs_to_rx;
        while (msgs_to_rx > 0) {
            msgs_to_rx--;
            twai_message_t rx_msg;
            ESP_ERROR_CHECK(twai_receive(&rx_msg, portMAX_DELAY));
            switch(rx_msg.identifier) {
                // default:
                //     log_can_event(&rx_msg, 0xffffffffffffffff);
                //     break;
                // Watched PIDs & their masks
                case 0x208:
                    log_can_event(&rx_msg, 0x000000ff00000000);
                    break;
                case 0x349:
                    log_can_event(&rx_msg, 0xf000000000000000);
                    break;
                case 0x38d:
                    log_can_event(&rx_msg, 0xffff000000000000);
                    break;
                case 0x3c9:
                    log_can_event(&rx_msg, 0x0000ff0000000000);
                    break;
                case 0x488:
                    log_can_event(&rx_msg, 0xff00000000ff00ff);
                    break;
            }
        }
        vTaskDelay(1);
    }
    xSemaphoreGive(rx_sem);
    vTaskDelete(NULL);
}
