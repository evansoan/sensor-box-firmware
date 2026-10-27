# For now, I'm not adding the diagnostic mode or the sensor-program mode
idf_component_register(SRCS "main.c" "sensor-box-wifi.c" "sensor-read.c" "sensor-box-serial.c"
                       PRIV_REQUIRES spi_flash esp_wifi nvs_flash esp_hw_support esp_driver_gpio esp_timer ina219 esp-modbus esp_netif esp_driver_ledc lwip esp_pm esp_http_client
                       INCLUDE_DIRS ".")
