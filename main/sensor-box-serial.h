#ifndef SENSOR_BOX_SERIAL_H
#define SENSOR_BOX_SERIAL_H

#include "sensor-box.h"
#include "mbcontroller.h"

/* I've placed all the UART helper functions here. Doing raw uart seems annoying, so I'll
 * abstract it into this file.
 *
 *
 *
 */


// For UART0 [connected to the serial header
// for debugging (and maybe eventually remote configuration/parameter updating)
#define UART0_RX_BUF_SIZE 2048
#define UART0_TX_BUF_SIZE 2048




// This just combines the sprintf and uart_write commands into a single function. It uses
// a buffer size of 1000 chars, which should be enough for basically anything we'd need
// to print. It also includes code to prevent a buffer overwrite, but I haven't validated
// it or anything.
void uart_printf(const char* data, ... );


// The RS-485 stuff can also go here

#define OPTS(min_val, max_val, step_val) { .opt1 = min_val, .opt2 = max_val, .opt3 = step_val }

//NOTE: So, esp-modbus requires us to build a table of all the characteristics that we
//will use. Unlike ModbusMaster for arduino, which just had you request registers by a
//numeric offset for a given slave device address

// I based this table (and my modbus api interactions) pretty heavily on the example code
// provided by espressif

enum {
    MB_DEVICE_CO2 = 1,
    MB_DEVICE_LIGHT = 2,
    MB_DEVICE_SOIL = 3,
    MB_DEVICE_LEAF = 4,
    MB_DEVICE_WIND_SPEED = 5,
    MB_DEVICE_WIND_DIR = 6
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum {
    CID_AIR_CO2 = 0,
    CID_AIR_TEMP,
    CID_AIR_HUMIDITY,
    CID_LIGHT_HIGHWORD,
    CID_LIGHT_LOWWORD,
    CID_LIGHT_STATUS,
    CID_SOIL_MOISTURE,
    CID_SOIL_TEMP,
    CID_SOIL_EC,
    CID_LEAF_TEMP,
    CID_LEAF_MOISTURE,
    CID_WIND_SPEED,
    CID_WIND_DIR,
    CID_COUNT
};

static const mb_parameter_descriptor_t device_parameters[] = {
// { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size, Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    {   //CO2 measurement
        CID_AIR_CO2, "CO2", "ppm", MB_DEVICE_CO2, MB_PARAM_HOLDING,
        0, 1,
        0, PARAM_TYPE_U16, 2,
        OPTS(0, 0, 0), PAR_PERMS_READ_WRITE_TRIGGER
    },
    {   //air temp, in centi degrees Celsius, stored in 2s complement
        CID_AIR_TEMP, "AIRTEMP", "c°C", MB_DEVICE_CO2, MB_PARAM_HOLDING,
        1, 1,
        1, PARAM_TYPE_U16, 2,
        OPTS(0, 0, 0), PAR_PERMS_READ_WRITE_TRIGGER
    },
    {   //air humidity, in hundredths of a percent
        CID_AIR_HUMIDITY, "Humidity", "c%", MB_DEVICE_CO2, MB_PARAM_HOLDING,
        2, 1,
        2, PARAM_TYPE_U16, 2,
        OPTS(0, 0, 0), PAR_PERMS_READ_WRITE_TRIGGER
    }

};
static const uint16_t num_device_parameters = (sizeof(device_parameters) / sizeof(device_parameters[0]));


static void *master_handle = NULL;

esp_err_t modbus_init(void);

//return value is a status code. Data must be at least as long as len, otherwise memory
//corruption could occur. Regnum is the start register to read.
esp_err_t modbus_read_registers(uint16_t cid, uint16_t* data);



#endif
