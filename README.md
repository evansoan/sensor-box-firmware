# Sensor Box Code

This is the firmware for the sensor boxes for the Michelman Agrivoltaics project

This version is a rewrite using the esp-idf toolchain in raw C.

## Why refactor the code?
This refactor was done for several reasons. First, I wanted to have low-level access to
raw device features, such as watchdog timers.

Second, I wanted to get rid of the overhead for the arduino libraries.

Third, the arduino-ide is pretty limiting in terms of code structure and organization. By
migrating, I can split the code up between headers and code files and make the code much
easier to work with

I used the esp-idf-lib project to source drivers for some of the sensors


# MIGRATION NOTES:
digitalWrite is replaced by ```gpio_set_level()```

to configure a GPIO pin as output, do this:
```
gpio_reset_pin(PIN_NUM);
gpio_set_direction(PIN_NUM, GPIO_MODE_OUTPUT);
```

digitalRead is replaced by 
```
gpio_get_level(PIN_NUM);
```
delay(milli) is replaced by 
```
vTaskDelay(milli / portTICK_PERIOD_MS);
```

Modbus notes:
CID (characteristic identifier) is a specific modbus register to be read or written

A Data dictionary is a list that defines all registers on all slave devices in network

# THINGS THAT STILL NEED TO BE MIGRATED!!!
  diagnostic mode wakeup trigger (low priority)
  http connection to send data
    * THIS IS VERY HIGH PRIORITY!
  restructure packet generation to use string.h functions! [high priority]
  all IO Stuff
    * INA219  [done, untested]
    * SGP30   [not done.]
    * modbus sensors  [partially done, untested]
    * color LED    [done, untested]
    * diagnostic mode stuff [this is commented out for now, as it's low priority]
    * serial  [done, untested]
  Time stuff (NTP, localtime, elapsedtime, etc)
  analog write? (i have to do pwm stuff) (only for LED colors)
  ERROR STUFF? (the functions from the sample code I've found use ESP_ERROR_CHECK. Is
      this the optimal way to handle this? Whatever the case, we need to make sure
      errors are explicitly handled in a graceful way

## Code structure
This is tentative, and I will likely do more reorganization during the migration process.

### sensor-box.h
This one includes all the libraries required, and defines constants. It may eventually
include function declarations for functions common to the whole project

### sensor-read [header/source]
This one handles actually reading data from the sensors, whether i2c, modbus, etc.

### sensor-box-serial [header/source]
This one has wrapper functions for the raw uart transfers, as well as probably some
other stuff with RS-485/modbus, but we'll see.
### sensor-box-wifi [header/source]
This one handles all the wireless stuff, as well as preparing the JSON packets

### diagnostic [header/source]
This has all the stuff relating to the diagnostic mode

### sensor-program [header/source]
this will handle the sensor address programming mode, once I finish writing it




## New Features

Our updates include adding the diagnostic mode, the sensor programming mode, and adding
new sensors.

## TODO list:

Things that still need to be done:
* make sure the diagnostic mode is finished.
* Migrate all the libraries from arduino to esp-idf


## Build Instructions

To build the project, you must have the esp-idf toolchain installed


## Credits

The original version of the code was written by a previous team using Arduino.

This version was written by Adam E.
