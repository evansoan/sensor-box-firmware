
// OMID sensor box firmware, esp-idf version
// this is a modification of the arduino-ide version of the firmware, but has been reorganized and refactored, and migrated off of the arduino libraries
// migration was done by A. Evanson, the original version of the code was written by previous students who worked on this project.
// further development done by: ###

// TODO: All serial stuff has to be rewritten to use basic esp serial
// also, try to remove c++ functionality and restructure it to use only c stuff

#include "sensor-box.h"
#include "sensor-box-serial.h"
#include "sensor-box-wifi.h"

// DEFINITIONS:
// I had to place them here and include an extern directive in the header, otherwise the
// compiler got mad at me

const char* ssid     = "Main";
const char* password = "5139191357";

// Cloud Function Endpoint

// Organization, Site, and Zone IDs
const char* ORGANIZATION_ID = "GS6e4032WK70vQ42WTYc";
const char* SITE_ID         = "5LvgyAAaFpAmlfcUrpTU";
const char* ZONE_ID         = "BKwm5W3qxjee8cIHvSQx";  //differs between zones
const short ZONE_NUM = 2;  //likewise differs between zones

// Sensor IDs

// This whole section needs to be reworked, as several sensors were added & removed
//
const char* SENSOR_ID_SOIL          = "vpHzjeTQMc7fUdzUqmFN";
const char* SENSOR_ID_CO2_TVOC      = "8jTNyQ7lR1fgOjoOT5Yt";
const char* SENSOR_ID_BATTERY       = "LiVAa6Oj2CYpiIlGTuUA";

const char* SENSOR_ID_WIND_DIR      = "C2BP6ISRAa34eWEHOuwn";
const char* SENSOR_ID_WIND_SPEED    = "DiqIKJ7KykTOWd4SXxNK";
const char* SENSOR_ID_LEAF          = "O2AohIUWUOe7Sn8VnjcZ";
const char* SENSOR_ID_LIGHT_485     = "sUvpUqZoS1XpBfUdicND"; //reading isn't added to the webapp yet
const char* SENSOR_ID_CO2_T_H       = "dxIjJR4wp7xhZGVFxRAR";

// Unit Constants
const char* UNIT_FAHRENHEIT = "°F";
const char* UNIT_PERCENT    = "%";
const char* UNIT_LUX        = "lux";
const char* UNIT_PPM        = "ppm";
const char* UNIT_PPB        = "ppb";
const char* UNIT_MOISTURE   = "VWC%";
const char* UNIT_EC         = "μS/cm";
const char* UNIT_VOLT       = "V";
const char* UNIT_AMP        = "A";

const char* UNIT_M_S        = "m/s";
const char* UNIT_DEGREE     = "°";


// -----------------------------
// LED Pins
// -----------------------------
#define COMMON_ANODE true
const gpio_num_t redPin   = 12; //was 0
const gpio_num_t greenPin = 14; //was 18
const gpio_num_t bluePin  = 27; //was 19


// --------------
// DIAGNOSTIC MODE PINS
// --------------

const gpio_num_t d1pin = 26; //was 17    //  ALL UPDATED
const gpio_num_t d2pin = 25; //was 34;  now is the DHT pin
const gpio_num_t d3pin = 18; //was 35;  now is SPARE_IO_2
const gpio_num_t sw1pin = 2;
const gpio_num_t sw2pin = 21; //was 15
const gpio_num_t RS485_DIR_PIN = 5;




uint32_t millis(void) {

  return (uint32_t) esp_timer_get_time() / 1000;
}

// -----------------------------
// Timing
// -----------------------------
RTC_DATA_ATTR uint32_t lastWakeTime = 0;

// -----------------------------
// RTC Memory for SGP30 baseline
// -----------------------------
RTC_DATA_ATTR uint16_t sgp30_baseline_CO2  = 0;  // This can be removed
RTC_DATA_ATTR uint16_t sgp30_baseline_TVOC = 0;
RTC_DATA_ATTR bool baseline_saved          = false;

// -----------------------------
// RTC Memory for LED state
// -----------------------------
RTC_DATA_ATTR int lastLED_R = 0;
RTC_DATA_ATTR int lastLED_G = 0;
RTC_DATA_ATTR int lastLED_B = 255;


// This is the instance of Reading that all data is written to. 

// initializes all reading data to 0
void init_reading(struct Reading* sd) {
  sd->tempF = 0;
  sd->humidity = 0;
  sd->soilMoistureRaw = 0;
  sd->soilMoisturePercent = 0;
  sd->soilTempF = 0;
  sd->soilEC = 0;
  sd->luxValue = 0;
  sd->co2 = 400;
  sd->tvoc = 0;
  sd->windSpeed = 0;
  sd->windDir = 0;
  sd->Voltage = 0;
  sd->Current = 0;
  sd->leafTemp = 0;
  sd->leafMoisture = 0;
}


// This tracks any sensor errors. It's used like a bitfield. If a sensor errors, it sets
// the corresponding bit. The errors can be cleared from diagnostic mode.
//
// TODO:
// As of now, the only place this is used is in diagnostic mode. It could be a good idea
// for the data packet to include sensor error information, or make it so a sufficient
// number of errors triggers a forced reboot (This would require a failsafe to prevent
// getting locked into a boot loop. Maybe this can only happen once the board's been on
// for a certain time period)
static uint16_t sensorErrors = 0; //one bit for each sensor
// sensor codes:
/*
 * 0 - INA219
 * 1 - SGP30
 * 2 - Modbus CO2 sensor
 * 3 - Modbus Light sensor
 * 4 - Modbus Soil sensor
 * 5 - Modbus Leaf Moisture sensor
 * 6 - Modbus Wind Speed sensor
 * 7 - Modbus Wind Dir sensor
 * 8 - Other error

*/




void preTransmission() {
  gpio_set_level(RS485_DIR_PIN, 1);
}

void postTransmission() {
  gpio_set_level(RS485_DIR_PIN, 0);
}

// this is a utility function that converts a uint16_t stored using 2's complement
int16_t uintToint(uint16_t in) {
  if(in > 32767) { //it's a negative number
    return (int16_t)( 1+ (~in) );   //to undo 2's complement, complement the bits and add 1. We then static_cast to a float
  }
  return in;
}

// -----------------------------
// Time / RTC helpers
// -----------------------------
// TODO: This all has to be rewritten.
bool waitForTime(uint32_t timeoutMs) {
  struct tm timeinfo;
  uint32_t start = millis();
  time_t now;

  while(millis() - start <= timeoutMs) {
    time(&now);
    localtime_r(&now, &timeinfo);
    if( timeinfo.tm_year > (2016 - 1990)) {
      return true;
    }
    vTaskDelay(10);
  }
  return false;

}

//this one initializes NTP. If it fails before timeoutMs, return false
bool ntp_init(uint32_t timeoutMs) {
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
  esp_netif_sntp_init(&config);

  if(esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000)) != ESP_OK) {
    uart_printf("Failed to update system time within 10s timeout");
    return false;
  }
  return true;
}



void setLEDColor(int r, int g, int b) {   // update this for blinking LED
  lastLED_R = r;
  lastLED_G = g;
  lastLED_B = b;

  if (COMMON_ANODE) {
    r = 255 - r;
    g = 255 - g;
    b = 255 - b;
  }

  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r));
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g));
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b));
  
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2));
}

// --------------------------------------------------
// I don't know what has to happen with all this.
// Commented out for now, until I migrate to the right library for my sensor
/*
void restoreSGP30Baseline() {
  if (baseline_saved) {
    sgp.setIAQBaseline(sgp30_baseline_CO2, sgp30_baseline_TVOC);
  }
}

// --------------------------------------------------
void saveSGP30Baseline() {
  uint16_t bCO2, bTVOC;
  if (sgp.getIAQBaseline(&bCO2, &bTVOC)) {
    sgp30_baseline_CO2 = bCO2;
    sgp30_baseline_TVOC = bTVOC;
    baseline_saved = true;
  }
}

*/



//setup:
// wifi setup
// I/O setups
//  including I2C, RS-485
// modbus setup
// NTP fetch

//TODO: All the library stuff here needs to be reworked

void setup() {
  init_reading(&sensorData); //clears the sensorData struct

  // Init the first UART Port (for serial logging and debugging.
  uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
  };
  int intr_alloc_flags = 0;
  ESP_ERROR_CHECK(uart_driver_install(0, UART0_RX_BUF_SIZE, UART0_TX_BUF_SIZE, 0, NULL,
        intr_alloc_flags));
  ESP_ERROR_CHECK(uart_param_config(0, &uart_config));
  ESP_ERROR_CHECK(uart_set_pin(0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  uint8_t *uart0_rxdata = (uint8_t *) malloc(UART0_RX_BUF_SIZE);
  uint8_t *uart0_txdata = (uint8_t *) malloc(UART0_TX_BUF_SIZE);

  //TODO: If we decide to add remote control over serial, this will need to be modified
  //to support an event queue. But that's not a high priority task, so no pressure


  //setup all general-use IO pins (the ones used by peripherals are managed elsewhere
  gpio_set_direction(redPin, GPIO_MODE_OUTPUT);
  gpio_set_direction(greenPin, GPIO_MODE_OUTPUT);
  gpio_set_direction(bluePin, GPIO_MODE_OUTPUT);
  gpio_set_direction(sw1pin, GPIO_MODE_INPUT);
  gpio_set_direction(sw2pin, GPIO_MODE_INPUT);
  gpio_set_direction(d1pin, GPIO_MODE_OUTPUT);
  gpio_set_direction(d2pin, GPIO_MODE_OUTPUT);
  gpio_set_direction(d3pin, GPIO_MODE_OUTPUT);

  
  // flash D1 briefly to verify operation. I can't remember exactly why I added this to
  // the prior version of the code. I think it was for debugging reasons.
  gpio_set_level(d1pin, 1);
  vTaskDelay(500 / portTICK_PERIOD_MS);
  gpio_set_level(d1pin, 0);

  // -----
  // Setup PWM for LED
  // -----

  ledc_timer_config_t ledc_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT, //matches the resolution of arduino analogWrite
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 400,
    .clk_cfg = LEDC_USE_RC_FAST_CLK
  };
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_config));

  ledc_channel_config_t ledc_channel_red = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .gpio_num = redPin,
    .duty = 255,
    .hpoint = 0, //phase offset, doesn't matter here
    .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,   //we want LED to remain on during sleep
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_red));

  ledc_channel_config_t ledc_channel_green = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_1,
    .timer_sel = LEDC_TIMER_0,
    .gpio_num = greenPin,
    .duty = 255, //init duty to 0%
    .hpoint = 0, //phase offset, doesn't matter here
    .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,   //we want LED to remain on during sleep
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_green));

  ledc_channel_config_t ledc_channel_blue = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_2,
    .timer_sel = LEDC_TIMER_0,
    .gpio_num = bluePin,
    .duty = 255, //init duty to 0%
    .hpoint = 0, //phase offset, doesn't matter here
    .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,   //we want LED to remain on during sleep
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_blue));

  setLEDColor(lastLED_R, lastLED_G, lastLED_B);

  // -----------------------------
  // WiFi first (needed for NTP)   [[ rewrite this whole thing]
  // -----------------------------
  wifi_init_sta();
  //WiFi.mode(WIFI_STA);
  //WiFi.setSleep(true);
  //WiFi.begin(ssid, password);

  unsigned long wifiStart = millis();
  const unsigned long wifiTimeout = 10000;

  wifi_ap_record_t ap_info;

  while (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK && millis() - wifiStart < wifiTimeout) {
    vTaskDelay(200 / portTICK_PERIOD_MS);
    uart_printf("Connecting to WiFi for NTP...\n");
  }

  //check wifi status:
  esp_err_t wifi_status = esp_wifi_sta_get_ap_info(&ap_info);

  if(wifi_status == ESP_OK) {
    uart_printf("WiFi connected for NTP.\n");
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    setLEDColor(0, 0, 255);
  } else {
    uart_printf("WiFi not connected yet, NTP may fail initially.\n");
    setLEDColor(255, 0, 0);
  }

  // -----------------------------
  // Timezone + NTP (Eastern with DST)
  // -----------------------------
  setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0/2", 1);
  tzset();


  uart_printf("Waiting for NTP time...\n");
  if(!ntp_init(15000)) {
    uart_printf("NTP wait timeout, continuing with whatever time is available.\n");
  } else {
    uart_printf("Time acquired (local).\n");
  }
  //TODO: Add a replacement for these functions, once I get one working
  //configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  //waitForTime(15000);  // try up to 15s to get valid time

  // -----------------------------
  // I2C / Sensors
  // -----------------------------

  // sgp


  //setup the ina219

  memset(&ina219, 0, sizeof(ina219_t));

  ESP_ERROR_CHECK(ina219_init_desc(&ina219, 0x40, 0, 33, 32));
  ESP_ERROR_CHECK(ina219_init(&ina219));

  ESP_ERROR_CHECK(ina219_configure(&ina219, INA219_BUS_RANGE_32V, INA219_GAIN_0_125,
                                     INA219_RES_12BIT_1S, INA219_RES_12BIT_1S, INA219_MODE_CONT_SHUNT_BUS));

  //ESP_LOGI(TAG, "Calibrating INA219");

  ESP_ERROR_CHECK(ina219_calibrate(&ina219, 0.1f)); // 0.1f is the shunt resistance


  //the modbus setup is done in a different function
  ESP_ERROR_CHECK(modbus_init());

  //test each modbus sensor.

  uint16_t param_test;
  if( modbus_read_registers(CID_AIR_CO2, &param_test) != ESP_OK) {
    uart_printf("Error: Could not connect to co2 sensor (should be Modbus address 1\n");
  } else {
    uart_printf("modbus co2 sensor connection successful");
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  
  if(modbus_read_registers(CID_LIGHT_STATUS, &param_test) != ESP_OK) {
    uart_printf("Error: Could not connect to light sensor (should be Modbus address 2\n");
  } else {
    uart_printf("modbus light sensor connection successful");
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  if(modbus_read_registers(CID_SOIL_MOISTURE, &param_test) != ESP_OK) {
    uart_printf("Error: Could not connect to soil sensor (should be Modbus address 3\n");
  } else {
    uart_printf("modbus soil sensor connection successful");
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  if(modbus_read_registers(CID_LEAF_TEMP, &param_test) != ESP_OK) {
    uart_printf("Error: Could not connect to leaf sensor (should be Modbus address 4\n");
  } else {
    uart_printf("modbus leaf sensor connection successful");
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  if(modbus_read_registers(CID_WIND_SPEED, &param_test) != ESP_OK) {
    uart_printf("Error: Could not connect to wind speed sensor (should be Modbus address 5\n");
  } else {
    uart_printf("modbus wind speed sensor connection successful");
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  if(modbus_read_registers(CID_WIND_DIR, &param_test) != ESP_OK) {
    uart_printf("Error: Could not connect to wind direction sensor (should be Modbus address 5\n");
  } else {
    uart_printf("modbus wind direction sensor connection successful");
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);


  // -----------------------------
  // First boot: init buffer + lastWakeTime
  // -----------------------------
  if (lastWakeTime == 0) {  
    uint32_t now = time(nullptr);
    if (now < 100000) {  // if time still looks invalid, force a wait
      waitForTime(10000);
      now = time(nullptr);
    }

    lastWakeTime = now;
    uart_printf("Initial lastWakeTime set to: ");
    uart_printf("%s\n", lastWakeTime);

    for (int i = 0; i < MAX_BUFFERED_READINGS; i++) {
      bufferedReadings[i].valid = false;
    }
  }

  // -----------------------------
  // Re-check WiFi for normal operation
  // -----------------------------
  // TODO: This needs to be fixed
  ensureWiFiConnected();


  //if i don't do this again, the thing breaks
  gpio_set_pull_mode(sw1pin, GPIO_PULLUP_ONLY);

  vTaskDelay(500 / portTICK_PERIOD_MS);

  if(gpio_get_level(sw1pin) == 0) {
    //enter sensor address assignment mode
    //sensorAddressProgram(); //will not be used for now
  }
  if(gpio_get_level(sw2pin) == 0) {
    //enter serial configuration mode, once that's implemented
  }
}

// --------------------------------------------------
void loop() {
  // Make sure local time is still sane (quick check)
  waitForTime(2000);
  
  //if SW1 is pressed (or if it woke the device from sleep), enter diagnostic mode
  if(gpio_get_level(sw1pin) == 0) {
    //diagnosticMode(); //diagnostic mode hasn't been fixed yet. Do nothing for now
  }

  uint32_t now = time(nullptr);
  uint32_t elapsed = now - lastWakeTime;

  uart_printf("Elapsed awake+sleep time: ");
  uart_printf("%s\n", elapsed);

  bool wifiOK = ensureWiFiConnected();

  // -----------------------------
  // SGP30
  // -----------------------------
  /*
  sgp40_measure_voc(&sgp, 
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  uint32_t sumCO2 = 0, sumTVOC = 0;
  for (int i = 0; i < 5; i++) {
    sgp.IAQmeasure();
    //sumCO2  += sgp.eCO2;
    sumTVOC += sgp.TVOC;
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
  //co2  = sumCO2 / 5;
  sensorData.tvoc = sumTVOC / 5;
  uart_printf("SGP30 tvoc: %i\n", sensorData.tvoc);
  */
  
  vTaskDelay(1500 / portTICK_PERIOD_MS);

  // -----------------------------
  // Soil Sensor
  // -----------------------------
  /* COMMENTED UNTIL I VERIFY ONE SENSOR WORKS
  node.setSlaveAddress(0x03); //soil sensor is addr. 3
  if (node.readHoldingRegisters(0x00, 3) == node.ku8MBSuccess) {
    sensorData.soilMoistureRaw     = node.getResponseBuffer(0);
    sensorData.soilMoisturePercent = sensorData.soilMoistureRaw / 10.0;

    int16_t rawTemp = (int16_t)node.getResponseBuffer(1);
    float soilTempC = rawTemp / 10.0;
    sensorData.soilTempF       = soilTempC * 9.0 / 5.0 + 32.0;

    sensorData.soilEC = node.getResponseBuffer(2);
    uart_printf("Soil Sensor: temp=%f F, EC=%f\n", sensorData.soilTempF, sensorData.soilEC);
  } else {
    uart_printf("Soil sensor not responding\n");
    sensorErrors |= 0x10;
  }
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  */

  // ----
  // CO2/Temp/Humidity Sensor
  // ----
  bool CO2_error = false;
  uint16_t rawco2;
  if( modbus_read_registers(CID_AIR_CO2, &rawco2) != ESP_OK) {
    CO2_error = true;
    sensorData.co2 = 0;
  } else {
    sensorData.co2 = rawco2;
  }

  uint16_t rawTempUint;
  if( modbus_read_registers(CID_AIR_TEMP, &rawTempUint) != ESP_OK) {
    CO2_error = true;
    sensorData.tempF = 0;
  } else {
    int16_t fixed = (rawTempUint > 32767) ? (1+ ~rawTempUint) : rawTempUint;
    sensorData.tempF = 32 + ( (float) (fixed) * 1.8F / 100.f);
  }
  
  uint16_t rawHumidity;
  if( modbus_read_registers(CID_AIR_HUMIDITY, &rawHumidity) != ESP_OK) {
    CO2_error = true;
    sensorData.humidity = 0;
  } else {
    //the packet expects this to be a float, not a uint, so we'll cast it
    sensorData.humidity = 0.01 * (float)(rawHumidity);  
  }

  if(!CO2_error) {
    uart_printf("CO2 sensor: co2=%i ppm, temp=%f F, humidity = %f%%\n", sensorData.co2, sensorData.tempF, sensorData.humidity);
  } else {
    //error with sensor
    uart_printf("CO2 sensor not responding\n");
    sensorErrors |= 0x4;
  }
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  
  /*  COMMENTED OUT UNTIL I CAN GET ONE SENSOR DONE RIGHT
  
  // -----
  // Modbus Light Sensor
  // -----
  node.setSlaveAddress(0x2);
  if (node.readHoldingRegisters(0x00, 3) == node.ku8MBSuccess) {
    // illuminance (high word), illuminance (low word) [lux], status
    // the packet expects a float, so we need to do some funky business
    //we're just left-shifting the high word by 16 (width of the word), then bitwise or-ing it with the low word. Finally, we cast it to a float
    sensorData.luxValue = (float)(( node.getResponseBuffer(0) << 16) | (node.getResponseBuffer(1)));
    //the status value is as follows:
    uint16_t status = node.getResponseBuffer(2);
    bool offScaleHigh = (status & 1) == 1; // we're checking a specific bit of status. I think this is the best way to do it
    bool error = (status & 2) == 2;
    
    if(offScaleHigh) {
      sensorData.luxValue = 400000.f;
    }
    if(error) {
      sensorErrors |= 0x8;
      uart_printf("Light sensor error\n");
    }
    uart_printf("Light Sensor: light value=%f lux\n", sensorData.luxValue);
  } else {
    //error with sensor
    sensorErrors |= 0x8;
    uart_printf("Light sensor not responding\n");
  }
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  
  // -----
  // Modbus Leaf Moisture
  // -----
  node.setSlaveAddress(0x4);
  if (node.readHoldingRegisters(0x0, 2) == node.ku8MBSuccess) {
    //temperature (100 units per degree C, ranges from -4000 to 8000). We need to reinterpret as a int16_t to make this work
    //wetness percent (100 units per percent, ranges from 0 to 10000)
  
    //go through the process of converting this properly
    int16_t rawTemp = uintToint(node.getResponseBuffer(0));
    float leafTempC = (float)(rawTemp) / 100.f;
    sensorData.leafTemp = 32 + (1.8 * leafTempC);

    sensorData.leafMoisture = (float)(node.getResponseBuffer(1)) / 100.f; //once it's a float, divide by 100 to turn it to a percent.

    uart_printf("Leaf Sensor: temp=%f F, moisture=%f%%\n", sensorData.leafTemp, sensorData.leafMoisture);
  } else {
    sensorErrors |= 0x20;
    uart_printf("Leaf sensor not responding\n");
  }
  vTaskDelay(1500 / portTICK_PERIOD_MS);

  // -----
  // Modbus Wind Speed
  // -----
  node.setSlaveAddress(0x5);
  if (node.readHoldingRegisters(0x0, 1) == node.ku8MBSuccess) {
    //wind speed, in 0.1 m/s increments.
    //we just cast to float, then divide by 10 to get speed in m/s
    sensorData.windSpeed = (float)(node.getResponseBuffer(0)) / 10.0f;
    uart_printf("Wind speed=%f m/s, ",sensorData.windSpeed);
  } else {
    //error: report/record it
    sensorErrors |= 0x40;
    uart_printf("Wind speed sensor not responding\n");
  }
  vTaskDelay(1500 / portTICK_PERIOD_MS);

  // -----
  // Modbus Wind Direction
  // -----
  node.setSlaveAddress(0x6);
  if (node.readHoldingRegisters(0x0, 2) == node.ku8MBSuccess) {
    //wind direction, in tenths of a degree
    //wind direction, rounded to nearest of 16 main directions (like NE, ESE, W, etc.)
    
    sensorData.windDir = (float)(node.getResponseBuffer(0)) / 10.0f;
    uart_printf("Wind direction=%f degrees\n", sensorData.windDir);
  } else {
    //error: report/record it
    sensorErrors |= 0x80;
    uart_printf("Wind dir sensor not responding\n");
  }
  vTaskDelay(1500 / portTICK_PERIOD_MS);
  */

  // -----------------------------
  // Battery
  // -----------------------------
  ESP_ERROR_CHECK(ina219_get_bus_voltage(&ina219, &sensorData.Voltage));
  ESP_ERROR_CHECK(ina219_get_current(&ina219, &sensorData.Current));
  //sensorData.Voltage = ina219.getBusVoltage_V();
  //sensorData.Current = ina219.getCurrent_mA() / 1000.0;

  // -----------------------------
  // Capture REAL reading timestamp (local time, EST/EDT)
  // -----------------------------
  uint32_t readingTimestamp = time(nullptr);
  uart_printf("Reading timestamp: ");
  uart_printf("%s\n", readingTimestamp);

  // -----------------------------
  // Cloud upload with buffering
  // -----------------------------
  // disabled during initial debugging
  if (wifiOK) {
    // First, flush any buffered readings
    flushBufferedReadings();

    // Build live packet from current globals
    // TODO: change how this works
    struct ReadingPacket live;
    live.valid      = true;
    live.timestamp  = readingTimestamp;
    live.data = sensorData;

    bool valid;
    unsigned long timestamp;

    if (!sendPacket(&live)) {
      uart_printf("Live reading failed, buffering.\n");
      bufferFailedReading(readingTimestamp);
    }
  } else {
    uart_printf("WiFi not OK, buffering current reading.\n");
    bufferFailedReading(readingTimestamp);
  }

  //saveSGP30Baseline();
  gpio_set_pull_mode(sw1pin, GPIO_PULLUP_ONLY);

  // -----------------------------
  // RTC ELAPSED-TIME CORRECTION
  // -----------------------------
  now = time(nullptr);
  elapsed = now - lastWakeTime;

  int32_t remaining = INTERVAL_SECONDS - (int32_t)elapsed;
  if (remaining < 5) remaining = 5; // safety minimum

  uart_printf("Sleeping for (s): ");
  uart_printf("%s\n", remaining);
  vTaskDelay(1500 / portTICK_PERIOD_MS);

  lastWakeTime = now;

  //standard timer wakeup
  esp_sleep_enable_timer_wakeup((uint64_t)remaining * uS_TO_S_FACTOR);

  //wakeup if diagnostic mode PB is pressed. Make sure the static_cast works the way I think...
  gpio_wakeup_enable(sw1pin, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  esp_light_sleep_start();
}


// initialize wifi

// LOOP:
//  check for input combinations to go to diagnostic mode. If so, go to that now
// upon wakeup, determine time
// poll sensor data
// create packet

void app_main(void) {

  // the wifi sample code uses NVS. I'm not entirely sure what it's for, but I'm
  // including it to be safe
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
  wifi_init_sta();
  
  setup();

  while(true) {
    loop();
  }
}
