#ifndef SENSOR_BOX_H
#define SENSOR_BOX_H
// libraries needed:


//standard c libraries
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

//esp-idf libraries
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "ina219.h"
//#include "sgp40.h"
#include "mbcontroller.h"
#include "modbus_params.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h" //for ntp

#include "driver/gpio.h" //for basic gpio stuff
#include "driver/uart.h" //for serial comms
#include "driver/ledc.h" //for LED PWM output
#include "driver/i2c_master.h" //for i2c
#include "esp_log.h"
#include "esp_pm.h"

//main esp-idf includes, whatever those are

//http client
//modbus for esp-idf

//handled by esp32 i2c base functionality:
//ina219, veml7700, sgp30

#define EXAMPLE_SHUNT_RESISTOR_MILLI_OHM 100
#define EXAMPLE_I2C_ADDR 0x40


// overall components:
// library & variable setup



// Organization, Site, and Zone IDs
#define ORGANIZATION_ID "GS6e4032WK70vQ42WTYc"
#define SITE_ID "5LvgyAAaFpAmlfcUrpTU"
#define ZONE_ID "QMtytS510w5B9iJ2Z0C4"
#define ZONE_NUM 3

// NOTE: I changed this code from zone 2 to zone 3 for the purpose of testing. I don't
// want this code to pollute the zone2 data, and since zone3 is offline, i thought it'd
// be a good choice.

//const char* ZONE_ID         = "BKwm5W3qxjee8cIHvSQx";  //differs between zones
//const short ZONE_NUM = 2;  //likewise differs between zones

// Sensor IDs

// This whole section needs to be reworked, as several sensors were added & removed
//
#define SENSOR_ID_SOIL "vpHzjeTQMc7fUdzUqmFN"
#define SENSOR_ID_CO2_TVOC "8jTNyQ7lR1fgOjoOT5Yt"
#define SENSOR_ID_BATTERY "LiVAa6Oj2CYpiIlGTuUA"

#define SENSOR_ID_WIND_DIR "C2BP6ISRAa34eWEHOuwn"
#define SENSOR_ID_WIND_SPEED "DiqIKJ7KykTOWd4SXxNK"
#define SENSOR_ID_LEAF "O2AohIUWUOe7Sn8VnjcZ"
#define SENSOR_ID_LIGHT_485 "sUvpUqZoS1XpBfUdicND"
#define SENSOR_ID_CO2_T_H "dxIjJR4wp7xhZGVFxRAR"

// Unit Constants
#define UNIT_FAHRENHEIT "°F"
#define UNIT_PERCENT "%"
#define UNIT_LUX "lux"
#define UNIT_PPM "ppm"
#define UNIT_PPB "ppb"
#define UNIT_MOISTURE "VWC%"
#define UNIT_EC "μS/cm"
#define UNIT_VOLT "V"
#define UNIT_AMP "A"
#define UNIT_M_S "m/s"
#define UNIT_DEGREE "°"

#define SSID "Main"
#define PASSWORD "5139191357"

// -----------------------------
// LED Pins
// -----------------------------
#define COMMON_ANODE true
#define redPin 12
#define greenPin 14
#define bluePin 27


// --------------
// DIAGNOSTIC MODE PINS
// --------------

#define d1pin 26
#define d2pin 25
#define d3pin 18
#define sw1pin 2
#define sw2pin 21
#define RS485_DIR_PIN 5


// -----------------------------
// Timing
// -----------------------------
#define INTERVAL_SECONDS 900
#define uS_TO_S_FACTOR 1000000ULL

// a wrapper for esp_timer_get_time() that returns the time in milliseconds, to preserve
//
uint32_t millis(void);

// global sensor variables:
static ina219_t ina219;



// -----------------------------
// Sensor Data Reading
// -----------------------------      
//! This struct contains all the sensor measurements for one reading. It's used for
struct Reading {
  float tempF;                 //temp in F
  float humidity;              //the new sensor reports this as a uint16_t
  uint16_t soilMoistureRaw;    //unchanged
  float soilMoisturePercent;   //unchanged
  float soilTempF;             //unchanged
  uint16_t soilEC;             //unchanged
  float luxValue;              //needs to change to a uint32_t
  uint16_t co2;                //unchanged, but needs to be fetched from a new sensor
  uint16_t tvoc;               //unchanged
  float windSpeed;             // m/s
  float windDir;               //angle, in degrees
  float Voltage;               //unchanged
  float Current;               //unchanged
  float leafTemp;              //new value. Measured in °F
  float leafMoisture;          //new value. Percent, from 0 to 100
};

static struct Reading sensorData;

// -----------------------------
// Missed Upload Buffer (RTC)
// -----------------------------
#define MAX_BUFFERED_READINGS 6

struct ReadingPacket { //so, I have A struct here, but need to consolidate all this stuff
  bool valid;
  unsigned long timestamp;
  struct Reading data;
};

static RTC_DATA_ATTR struct ReadingPacket bufferedReadings[MAX_BUFFERED_READINGS];

void setLEDColor(int r, int g, int b);

#endif
