//

#include "sensor-box.h"
#include "sensor-program.h" //doesn't exist yet...


// Which modbus register holds the slave address
const uint16_t regAddr[] = {
  0x0,   //dummy value to pad the array (since we start with address 1)
  0x10,  //Temp/CO2/Humidity  (will become address 1)
  0x200, //Light (will become address 2)
  0x7D0, //Soil sensor (will become address 3)
  0x200, //Leaf Moisture sensor (will become address 4)
  0x1000,//Wind Speed sensor (will become address 5)
  0x1000,//Wind direction sensor (will become address 6)
};

// which default address each sensor uses
const uint16_t regOldAddr[] = {
  0x0,  //dummy value to pad the array
  45,   //temp/co2/humidity
  1,    //light
  1,    //soil
  1,    //leaf wetness
  2,    //wind speed
  2     //wind direction
};

void sensorAddressProgram() {
  Serial.println("Entering Sensor Programming Mode");
  node.setSlaveAddress(45); //we're trying to write to the broadcast address
  //basically, this
  uint16_t addr = 1; //address 0 is reserved for broadcast, so the first sensor gets address 1
  unsigned short loopNum=0;  //used to count loop iterations for timing
  uint8_t ledCode=1; // [xxxxxABC], where ABC are the three diagnostic LEDs, respectively
  uint8_t r=0;
  uint8_t g=0;
  uint8_t b=0;
  bool blink=false;
  bool displayedMsg=false; //has the message announcing sensor been displayed yet?

  unsigned short state=0;
  //0 - idle
  //1 - fault
  //2 - cooldown
  //3 - out-of-bounds address

  pinMode(sw2pin, INPUT);
  
  while(true) {
    //we'll display a led code
    if(displayedMsg == false) {
      Serial.printf("Please connect sensor %i\n", addr);
      displayedMsg = true;
    }
    if(digitalRead(sw1pin) == LOW && loopNum == 0) { //only test state changes on 1 second intervals
      //button has been pressed. Try to program the sensor on address '0'
      // I looked into it, and address 0 is the 'broadcast' address, so all connected devices
      // will respond
      // however, the address register is stored in different places for the different sensors.
      // Thus, we either need to try writing to each possible register, or determine which device is currently connected...somehow?
      
      if(addr > 6) {
        //error: BAD BAD BAD
        r = 255;
        g = 127;
        b = 0;
        state = 3;
        continue;

        
      }
      uint16_t targetAddr = regAddr[addr];
      
      uint16_t defaultAddr = regOldAddr[addr];
      
      node.setSlaveAddress(defaultAddr);
      Serial.printf("Attempting to program sensor %i [modbus regno = %i]\n", addr, targetAddr);
      auto status = node.writeSingleRegister(targetAddr, addr);
      if(status == node.ku8MBSuccess) {
        //success.
        r=0;
        g=255;
        b=0;
        addr++;
        if(addr == 7) { //after the increment, 7 is the maximum
          state = 4;
          break;
        }
        state = 2; //cooling down before moving on to next sensor
        Serial.printf("Programming successful. Please disconnect sensor\n");
        displayedMsg = false;
      } else {
        //failure.
        r=255;
        g=0;
        b=0;
        //enter fault state
        state = 1;
        Serial.printf("Error with programming sensor %i. Press SW1 to retry, press SW2 to skip\n[error code is %x]\n", addr, status);
      }
    }
    
    if(loopNum == 0 && state == 1) {
      //handle failure. If SW1 is pressed, retry
      // If SW2 is pressed, skip and move on to next time
      // If both buttons are pressed, do nothing.
      if(digitalRead(sw1pin) == LOW && digitalRead(sw2pin) == HIGH) {
        //retry programming
        Serial.printf("Retrying\n");
      } else if(digitalRead(sw1pin) == HIGH && digitalRead(sw2pin) == LOW) {
        //move on to next address;
        addr++;
        Serial.printf("Moving on to next address\n");
      }
      state = 0;
      r=255;
      g=255;
      b=0;
    }
    if(loopNum == 0 && state == 2) {
      state = 0; //end cooldown after one cycle
    }

    if(loopNum == 0) { //switch to primary LED color
      setLEDColor(r, g, b);
    }
    if(loopNum == 5 && blink) { //switch to secondary LED color
      setLEDColor(0, 0, 0);
    }
    digitalWrite(d1pin, ledCode & 0x4);
    digitalWrite(d2pin, ledCode & 0x2);
    digitalWrite(d3pin, ledCode & 0x1);
    loopNum = (loopNum+1)%10;
    delay(100);
  }
}

