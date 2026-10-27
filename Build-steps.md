# How to build & flash this project to the esp32 board

1. Setup the environment. For linux, run the source command
2. Connect the device, and determine which serial port it is connected on

3. For first time running the code:
    * idf.py set-target esp32
    * idf.py menuconfig

4. Run idf.py build

5. Place the board into debug mode: Using the serial breakout board, Hold the
    * Given that the board was just something I threw together

5. Run idf.py -p [PORT] flash
6. Run idf.py -p [PORT] monitor
