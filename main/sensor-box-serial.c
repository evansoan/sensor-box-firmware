#include "sensor-box-serial.h"


void uart_print(const char* data, size_t len) {
  uart_write_bytes(0, data, len);
}

void uart_printf(const char* data, ... ) {
  //we don't know how long the buffer will need to be yet, We can just make an estimate.
  //Yes, this is wasteful, but the alternative is annoying.
  char buffer[1001];
  memset(buffer, 0, sizeof(buffer));

  va_list args;
  va_start(args, data); //I think this is right

  vsnprintf(buffer, sizeof(buffer), data, args); //this should prevent overwrites
  if(strlen(buffer) >= sizeof(buffer)) {
    uart_write_bytes(0, buffer, sizeof(buffer));
  } else {
    uart_write_bytes(0, buffer, strlen(buffer));
  }

  va_end(args);
}

esp_err_t modbus_init(void) {
  mb_communication_info_t comm = {
    .ser_opts.port = 1,
    .ser_opts.mode = MB_ASCII,
    .ser_opts.baudrate = 9600,
    .ser_opts.parity = MB_PARITY_NONE,
    .ser_opts.uid = 0, //? I have no idea what this means
    .ser_opts.response_tout_ms = 1000,
    .ser_opts.data_bits = UART_DATA_8_BITS,
    .ser_opts.stop_bits = UART_STOP_BITS_1,
  };
  ESP_ERROR_CHECK(mbc_master_create_serial(&comm, &master_handle));

  /*  I'M NOT SURE IF THIS ALL IS NEEDED [e.g. do I need a custom handler? I don't
   *  think so?]
  const uint8_t override_command = 0x41; //what does this do?
  ESP_ERROR_CHECK(mbc_delete_handler(master_handle, override_command));
  ESP_ERROR_CHECK(mbc_set_handler(master_handle, override_command, my_custom_handler));
  mb_fn_handler_fp handler = NULL;
  ESP_ERROR_CHECK(mbc_get_handler(master_handle, override_command, &handler));
  */

  //esp_err_t err = mbc_master_init(MB_PORT_SERIAL_MASTER, &master_handler);

  //ESP_ERROR_CHECK(mbc_master_setup( (void*)&comm));
  ESP_ERROR_CHECK(uart_set_pin(1, 4, 16, RS485_DIR_PIN, UART_PIN_NO_CHANGE));
  ESP_ERROR_CHECK(mbc_master_set_descriptor(master_handle, &device_parameters[0], num_device_parameters));
  ESP_ERROR_CHECK(uart_set_mode(1, UART_MODE_RS485_HALF_DUPLEX));
  vTaskDelay(5);

  return (mbc_master_start(master_handle));

}

esp_err_t modbus_read_registers(uint16_t cid, uint16_t* data) {
  //
  esp_err_t err = ESP_OK;
  const mb_parameter_descriptor_t *param_descriptor = NULL;
  uint8_t type = 0; //parameter type, should be 0.
  ESP_ERROR_CHECK(mbc_master_get_cid_info(master_handle, cid, &param_descriptor));

  return mbc_master_get_parameter(master_handle, cid, (uint8_t *)data, &type);
}
