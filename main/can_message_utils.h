static uint64_t read_data(twai_message_t * rx_msg) {
    uint64_t data = 0;
    uint8_t shift = 7 * 8;
    for (uint8_t i = 0; i < rx_msg->data_length_code; i++) {
        data |= ((uint64_t) rx_msg->data[i]) << shift;
        shift -= 8;
    }
    return data;
}

static void print_message(uint32_t identifier, uint32_t data_length_code, uint64_t data) {
    ESP_LOGI(
        CAN_TAG,
        "%3x [%d] %08x%08x",
        identifier,
        data_length_code,
        (uint32_t) (data >> 32),
        (uint32_t) data
    );
}

static void print_messages_diff(
    uint32_t identifier,
    uint32_t data_length_code,
    uint64_t data0,
    uint64_t data1,
    uint64_t diff) {
    ESP_LOGI(
        CAN_TAG,
        "%3x [%d] %08x%08x -> %08x%08x ^:%08x%08x",
        identifier,
        data_length_code,
        (uint32_t) (data0 >> 32),
        (uint32_t) data0,
        (uint32_t) (data1 >> 32),
        (uint32_t) data1,
        (uint32_t) (diff >> 32),
        (uint32_t) diff
    );
}
