// this file contains all the wifi/cloud functions
#include "sensor-box-wifi.h"

extern struct Reading sensorData;

// From the example code. I think this is just a handler for the Wifi events, but I'm not
// fully sure.
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      uart_printf("retry to connect to the AP\n");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    uart_printf("Failed to connect to the AP\n");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    //ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    uart_printf("Got IP %s", IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

//initialize the wifi connection. This was based on the example code
void wifi_init_sta(void) {
  s_wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL,
        &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL,
        &instance_got_ip));

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = EXAMPLE_ESP_WIFI_SSID,
      .password = EXAMPLE_ESP_WIFI_PASS,
      /* Authmode threshold resets to WPA2 as default if auth mode threshold equals WIFI_AUTH_OPEN
       * and password matches WPA2 standards (password len => 8).
       * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
       * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
       * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
       */
      .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
      .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
      .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
#ifdef CONFIG_ESP_WIFI_WPA3_COMPATIBLE_SUPPORT
      .disable_wpa3_compatible_mode = 0,
#endif
    },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
  ESP_ERROR_CHECK(esp_wifi_start() );

  ESP_LOGI(TAG, "wifi_init_sta finished.");

}

// --------------------------------------------------
// WiFi reconnect (ONE ATTEMPT ONLY)
// --------------------------------------------------     // possibly alter LED COLORs here
bool ensureWiFiConnected() {
  wifi_ap_record_t ap_info;
  esp_err_t wifi_status = esp_wifi_sta_get_ap_info(&ap_info);

  if(wifi_status == ESP_OK) {
    setLEDColor(0, 0, 255);
    return true;
  }

  uart_printf("WiFi disconnected. Trying one reconnect...\n");

  ESP_ERROR_CHECK(esp_wifi_disconnect());
  //WiFi.disconnect(true);
  vTaskDelay(200 / portTICK_PERIOD_MS);

  wifi_status = esp_wifi_connect();

  unsigned long wifiStart = millis();
  const unsigned long wifiTimeout = 8000;

  while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK && millis() - wifiStart < wifiTimeout) {
    vTaskDelay(200 / portTICK_PERIOD_MS);
    uart_printf("Connecting to WiFi for NTP...\n");
  }

  wifi_status = esp_wifi_sta_get_ap_info(&ap_info);

  if (wifi_status == ESP_OK) {
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    setLEDColor(0, 0, 255);
    return true;
  }

  uart_printf("WiFi failed (single attempt).\n");
  setLEDColor(255, 0, 0);
  return false;
}
/*  commented out so I can get the basic wifi stuff working

*/

// --------------------------------------------------
// Build and send JSON for a given packet (ONE ATTEMPT)
// --------------------------------------------------

// TODO: Test that the generated packet is still the same as with the old c++ code.
bool sendPacket(const struct ReadingPacket* p) {
  wifi_ap_record_t ap_info;
  esp_err_t wifi_status = esp_wifi_sta_get_ap_info(&ap_info);

  if(wifi_status != ESP_OK) {
    uart_printf("Wifi lost before packet upload.");
    setLEDColor(255, 0, 0);
    return false;
  }






  //pregeneration of some strings.
  char timestamp[30];
  char reading_buffer[50]; //50 is way overkill, given the snprintf settings we'll be
                           //using here
  sprintf(timestamp, "%lu", p->timestamp);
  //render timestamp into the buffer. Timestamp is a long, so max value is about 20
  //digits. I added a few extra in case I'm wrong, just for a safety margin.

  //I'm not sure how long the payload is expected to be, but I think it's pretty long.
  //do i want to just use a really long buffer, or try manual memory managemement?
  char packetbuffer[10000]; //10000 should be plenty overkill, but this can be tweaked.
  
  //preamble (organization stuff
  strcat(packetbuffer, "{");
  strcat(packetbuffer, "\"organizationId\":\"");
  strcat(packetbuffer, ORGANIZATION_ID);
  strcat(packetbuffer, "\",\"siteId\":\"");
  strcat(packetbuffer, SITE_ID);
  strcat(packetbuffer, "\",\"zoneId\":\"");
  strcat(packetbuffer, ZONE_ID);
  strcat(packetbuffer, "\",\"sensors\":[");
  
  //soil sensor
  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_SOIL);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"soilMoisture\":{\"value\":");
  snprintf(reading_buffer, 8, "%.2f", p->data.soilMoisturePercent); //generate moisture string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_MOISTURE);
  strcat(packetbuffer, "\"},");

  strcat(packetbuffer, "\"soilTemperature\":{\"value\":");
  snprintf(reading_buffer, 8, "%.2f", p->data.soilTempF); //generate soilTemp string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_FAHRENHEIT);
  strcat(packetbuffer, "\"},");

  strcat(packetbuffer, "\"soilEC\":{\"value\":");
  snprintf(reading_buffer, 10, "%i", p->data.soilEC); //generate soil EC string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_EC);
  strcat(packetbuffer, "\"}}},");

  //tvoc sensor  (I removed the co2 reading from here, as the modbus one is better)
  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_CO2_TVOC);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"tvoc\":{\"value\":");
  snprintf(reading_buffer, 10, "%i", p->data.tvoc); //generate tvoc string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_PPB);
  strcat(packetbuffer, "\"}}},");

  //ina219
  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_BATTERY);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"voltage\":{\"value\":");
  snprintf(reading_buffer, 8, "%.3f", p->data.Voltage); //generate voltage string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_VOLT);
  strcat(packetbuffer, "\"},");

  strcat(packetbuffer, "\"current\":{\"value\":");
  snprintf(reading_buffer, 8, "%.3f", p->data.Current); //generate current string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_AMP);
  strcat(packetbuffer, "\"}}},");


  //wind speed sensor

  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_WIND_SPEED);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"windSpeed\":{\"value\":");
  snprintf(reading_buffer, 8, "%.2f", p->data.windSpeed); //generate speed string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_M_S);
  strcat(packetbuffer, "\"}}},");
  
  //wind direction sensor
  
  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_WIND_DIR);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"windDirection\":{\"value\":");
  snprintf(reading_buffer, 10, "%.2f", p->data.windDir); //generate dir string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_DEGREE);
  strcat(packetbuffer, "\"}}},");

  //leaf moisture sensor

  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_LEAF);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"leafMoisture\":{\"value\":");
  snprintf(reading_buffer, 8, "%.2f", p->data.leafMoisture); //generate moisture string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_PERCENT);
  strcat(packetbuffer, "\"},");

  strcat(packetbuffer, "\"leafTemperature\":{\"value\":");
  snprintf(reading_buffer, 8, "%.2f", p->data.leafTemp); //generate temp string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_FAHRENHEIT);
  strcat(packetbuffer, "\"}}},");

  //modbus co2 sensor

  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_CO2_T_H);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"co2\":{\"value\":");
  snprintf(reading_buffer, 10, "%i", p->data.co2); //generate co2 string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_PPM);
  strcat(packetbuffer, "\"},");

  strcat(packetbuffer, "\"temperature\":{\"value\":");
  snprintf(reading_buffer, 10, "%.2f", p->data.tempF); //generate temperature string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_FAHRENHEIT);
  strcat(packetbuffer, "\"},");

  strcat(packetbuffer, "\"humidity\":{\"value\":");
  snprintf(reading_buffer, 10, "%.2f", p->data.humidity); //generate temp string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_PERCENT);
  strcat(packetbuffer, "\"}}},");

  //light sensor

  strcat(packetbuffer, "{\"sensorId\":\"");
  strcat(packetbuffer, SENSOR_ID_LIGHT_485);
  strcat(packetbuffer, "\",\"timestamp\":");
  strcat(packetbuffer, (const char*) timestamp);
  strcat(packetbuffer, ",\"readings\":{");

  strcat(packetbuffer, "\"light\":{\"value\":");
  snprintf(reading_buffer, 12, "%.2f", p->data.luxValue); //generate dir string
  strcat(packetbuffer, (const char*) reading_buffer);
  strcat(packetbuffer, ",\"unit\":\"");
  strcat(packetbuffer, UNIT_LUX);
  strcat(packetbuffer, "\"}}}]}");

  //now, I just need to figure out how to upload the packet.

  //sanity check to avoid buffer overflow. We can probably find a better way to handle
  //this. TODO: Maybe check at a couple points during the packet construction
  if(strlen(packetbuffer) >= sizeof(packetbuffer)) {
    uart_printf("Error: packet buffer overrun. Aborting!\n");
    return false;
  }


  //now, upload the request to remote server
  esp_http_client_config_t config = {
    .url = cloudFunctionURL,//endpoint
    //.event_handler = _http_event_handler,
    //.user_data

      //I'm not sure what handlers/response buffers I need. TODO: Figure that all out
  };
  esp_http_client_handle_t client = esp_http_client_init(&config);

  //POST request:

  esp_http_client_set_url(client, cloudFunctionURL);
  esp_http_client_set_method(client, HTTP_METHOD_POST);
  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_post_field(client, packetbuffer, strlen(packetbuffer));

  esp_err_t err = esp_http_client_perform(client);

  int httpCode = esp_http_client_get_status_code(client);

  esp_http_client_cleanup(client);

  //uart_printf("%s\n", jsonPayload); //for debugging
  if (err == ESP_OK && httpCode >= 200 && httpCode < 300) {
    uart_printf("Packet upload successful.\n");
    setLEDColor(0, 255, 0);
    return true;
  } else {
    uart_printf("Packet upload failed, HTTP code: %i\n", httpCode);
    setLEDColor(255, 0, 0);
    return false;
  }
}

// --------------------------------------------------
// Buffer a failed reading (current globals) with given timestamp
// --------------------------------------------------              
// ADD RELEVANT LED INDICATORS  (tbh i dont remember what i meant by this...
void bufferFailedReading(uint32_t readingTimestamp) {
  // Find first empty slot
  for (int i = 0; i < MAX_BUFFERED_READINGS; i++) {
    if (!bufferedReadings[i].valid) {
      bufferedReadings[i].valid      = true;
      bufferedReadings[i].timestamp  = readingTimestamp;
      bufferedReadings[i].data      = sensorData;

      uart_printf("Buffered failed reading at slot %i\n", i);
      return;
    }
  }

  // If buffer full, drop oldest (index 0) and shift
  uart_printf("Buffer full — dropping oldest and inserting newest.\n");
  for (int i = 1; i < MAX_BUFFERED_READINGS; i++) {
    bufferedReadings[i - 1] = bufferedReadings[i];
  }

  int idx = MAX_BUFFERED_READINGS - 1;
  bufferedReadings[idx].valid      = true;
  bufferedReadings[idx].timestamp  = readingTimestamp;
  bufferedReadings[idx].data = sensorData;

}


// --------------------------------------------------
// Flush buffered readings (oldest to newest)
// Wipes entries as soon as they are successfully sent
// Stops immediately on first failure (no retries)
// --------------------------------------------------                // ADD LED COLORS
void flushBufferedReadings() {
  for (int i = 0; i < MAX_BUFFERED_READINGS; i++) {
    if (bufferedReadings[i].valid) {
      uart_printf("Sending buffered reading from slot ");
      uart_printf("%i\n", i);

      if (sendPacket(&bufferedReadings[i])) {
        uart_printf("Buffered reading sent, clearing slot.\n");
        bufferedReadings[i].valid = false;  // wipe this entry
      } else {
        uart_printf("Buffered reading failed again, stopping flush.\n");
        return; // stop immediately — no retries this cycle
      }
    }
  }
}


