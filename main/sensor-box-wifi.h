#ifndef SENSOR_BOX_WIFI_H
#define SENSOR_BOX_WIFI_H

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"


#include "sensor-box.h"
#include "sensor-box-serial.h"

/*this portion of the project was inspired by espressif's wifi sample code, which can be
found here:

https://github.com/espressif/esp-idf/blob/master/examples/wifi/getting_started/station

*/



// wifi information:
#define EXAMPLE_ESP_WIFI_SSID "Main"
#define EXAMPLE_ESP_WIFI_PASS "5139191357"
#define EXAMPLE_ESP_MAXIMUM_RETRY 3 //TODO: what should this be?

#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

//cloud server information:
static const char* cloudFunctionURL = "https://us-central1-agrivoltaics-flutter-firebase.cloudfunctions.net/ingestSensorData";

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";
static int s_retry_num = 0;


void wifi_init_sta(void);



// Functions for Wifi and cloud stuff
void flushBufferedReadings();

void bufferFailedReading(uint32_t readingTimestamp);

bool sendPacket(const struct ReadingPacket* p);
bool ensureWiFiConnected();


#endif
