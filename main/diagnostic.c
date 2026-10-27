// this has all the stuff for diagnostic mode.

#include "sensor-box.h"

void diagnosticMode(void) {
  const unsigned short NUM_PAGES = 5;
  unsigned short mode=0; //which page of diagnostics is currently being displayed.
  unsigned short loopNum;  //used to count loop iterations for timing
  uint8_t ledCode; // [xxxxxABC], where ABC are the three diagnostic LEDs, respectively
  uint8_t r1=0;
  uint8_t g1=0;
  uint8_t b1=0;

  uint8_t r2=0;
  uint8_t g2=0;
  uint8_t b2=0;
  
  uint8_t sensErrCounter=0; //used to cycle through the sensor errors for display
  uint8_t numSensErrors=0; //how many sensors have errors
  uint8_t badSensors[9]; //numSensErrors is used to mark the high side
  
  bool s1lastpress = false; //was SW1 pressed last iteration?

  Serial.println("Entering Diagnostic Mode");
  pinMode(d1pin, OUTPUT);
  pinMode(d2pin, OUTPUT);
  pinMode(d3pin, OUTPUT);

  digitalWrite(d1pin, LOW); 
  digitalWrite(d2pin, LOW); 
  digitalWrite(d3pin, LOW); 
  delay(1000); //wait to prevent the mode from immediately switching

  //setup that only needs to be done once.
  for (int i = 0; i < 9; i++) {  //currently, there are 8 sensor codes & a misc code, for a total of 9.
    uint8_t bit = 1 << i;        //shift the active bit over each loop
    if(sensorErrors & bit) {
      badSensors[numSensErrors] = i;
      numSensErrors++;
    }
  }


  uint8_t buffered=0;
  for (uint8_t i = 0; i < MAX_BUFFERED_READINGS; i++) {
    if (!bufferedReadings[i].valid) {
      //this is our first empty slot
      buffered = i;
    }
  }

    //the actual diagnostic loop. Exits if both buttons are pressed at the same time
  do {
    //each loop sleeps for 0.10 s
    //poll for input every loop,

    if(digitalRead(sw1pin) == LOW && loopNum == 0) {
      mode = (mode+1) % NUM_PAGES;
      Serial.printf("Entering page %i\n", mode);
    }
    
    switch(mode) {
      case 0:
      //display number of packets in buffer

      ledCode = buffered & 0x7; //grab only the 3 LSB
      //set LED to Magenta / White
      r1 = 255; g1 = 255; b1 = 0;
      r2 = 255; g2 = 255; b2 = 255;
      break;
      case 1:
      //display connection strength
      {
        // query connection strength
        if(!WiFi.isConnected()) {
          if(loopNum < 5) {
            ledCode = 5;
          } else {
            ledCode = 2;
          }

        } else {
          int rssi=0;
          esp_wifi_sta_get_rssi(&rssi);
          if (rssi >= -50) {
            ledCode = 7;
          } else if (rssi >= -60) {
            ledCode = 3;
          } else if (rssi >= -80) {
            ledCode = 1;
          } else {
            ledCode = 0;
          }
        }
      }
      r1 = 0; g1 = 255; b1 = 255;
      r2 = 255; g2 = 255; b2 = 255;
      //set LED to Cyan / White
      break;
      case 2:
        //display sensor errors, if any
        //if multiple are set, cycle between them on a 1-second interval
        if(loopNum == 0 && numSensErrors != 0) {
          //update ledCode to next bad
          if(sensErrCounter == numSensErrors) {
            ledCode = badSensors[sensErrCounter];
          }
        } else {
          ledCode = 0;
        }
        
        //press SW2 to clear errors
        if(digitalRead(sw2pin) == LOW) {
          numSensErrors = 0;
          sensorErrors = 0;
          sensErrCounter = 0;

        }
      //set LED to White / Red
      r1 = 255; g1 = 255; b1 = 255;
      r1 = 255; g1 = 0; b1 = 0;
      break;
      case 3:
        {

          auto voltage = ina219.getBusVoltage_V();
          if(voltage >= 14) {
            ledCode = 7;
          } else if(voltage >= 12.8) {
            ledCode = 3;
          } else if(voltage >= 11.9) {
            ledCode = 1;
          } else {
            ledCode = 0;
          }
          
        }
      //set LED to Yellow / Green
      r1 = 255; g1 = 255; b1 = 0;
      r2 = 0; g2 = 255; b2 = 0;
      break;
      case 4:
        ledCode = ZONE_NUM & 0x7; //we only need the 3 LSB.
      //display programmed ZoneID
      //set LED to Cyan / Green
      r1 = 0; g1 = 255; b1 = 255;
      r2 = 0; g2 = 255; b2 = 0;
      break;
      default:
        mode = 0;
        //invalid mode reached. If serial is connected, report the error there?
        break;
    }

    if(loopNum == 0) { //switch to primary LED color
      setLEDColor(r1, g1, b1);
    }
    if(loopNum == 5) { //switch to secondary LED color
      setLEDColor(r2, g2, b2);
    }

    //display ledCode to appropriate GPIO pins
    //these pins may be in the wrong order, check this
    if(loopNum % 5 == 0) {
      digitalWrite(d1pin, (ledCode & 0x4) == 0x4);
      digitalWrite(d2pin, (ledCode & 0x2) == 0x2);
      digitalWrite(d3pin, ledCode & 0x1);
    }
    loopNum = (loopNum+1)%10;
    delay(100);
  }while(digitalRead(sw2pin) == HIGH || digitalRead(sw1pin) == HIGH); //either button is not pressed. 
  Serial.println("Leaving diagnostic mode");
  digitalWrite(d1pin, LOW); 
  digitalWrite(d2pin, LOW); 
  digitalWrite(d3pin, LOW); 

}
