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



// all the stuff here can go in a header, not in the actual

extern const char* ssid;
extern const char* password;

// Cloud Function Endpoint

// Organization, Site, and Zone IDs
extern const char* ORGANIZATION_ID;
extern const char* SITE_ID;
extern const char* ZONE_ID;  //differs between zones
extern const short ZONE_NUM;  //likewise differs between zones

// Sensor IDs

// This whole section needs to be reworked, as several sensors were added & removed
//
extern const char* SENSOR_ID_SOIL;
extern const char* SENSOR_ID_CO2_TVOC;
extern const char* SENSOR_ID_BATTERY;

extern const char* SENSOR_ID_WIND_DIR;
extern const char* SENSOR_ID_WIND_SPEED;
extern const char* SENSOR_ID_LEAF;
extern const char* SENSOR_ID_LIGHT_485; //reading isn't added to the webapp yet
extern const char* SENSOR_ID_CO2_T_H;

// Unit Constants
extern const char* UNIT_FAHRENHEIT;
extern const char* UNIT_PERCENT;
extern const char* UNIT_LUX;
extern const char* UNIT_PPM;
extern const char* UNIT_PPB;
extern const char* UNIT_MOISTURE;
extern const char* UNIT_EC;
extern const char* UNIT_VOLT;
extern const char* UNIT_AMP;

extern const char* UNIT_M_S;
extern const char* UNIT_DEGREE;



// -----------------------------
// LED Pins
// -----------------------------
#define COMMON_ANODE true
extern const gpio_num_t redPin; //was 0
extern const gpio_num_t greenPin; //was 18
extern const gpio_num_t bluePin; //was 19


// --------------
// DIAGNOSTIC MODE PINS
// --------------

extern const gpio_num_t d1pin; //was 17    //  ALL UPDATED
extern const gpio_num_t d2pin; //was 34;  now is the DHT pin
extern const gpio_num_t d3pin; //was 35;  now is SPARE_IO_2
extern const gpio_num_t sw1pin;
extern const gpio_num_t sw2pin; //was 15
extern const gpio_num_t RS485_DIR_PIN;


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
