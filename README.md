# VL53L1X sensor for CC2530

Inspired by https://github.com/pololu/vl53l1x-arduino

VL53L1X sensor wake ups every minute and reports distance in mm.

Built binary represents end device with power saving.

You need to place external converter (distance-sensor.js) into zigbee2mqtt.

Attempts to bind right after power on.

Pinout:

* P04 <-> SCL
* P05 <-> SDA

Requirements:

* IAR 10.10.1
* Z-Stack 3.0.2

Build instructions:

* Copy to Z-Stack 3.0.2\Projects\zstack\HomeAutomation
* Make binary
* Flash CC2530DB\EndDeviceEB\Exe\output.hex with TI SmartRF or something else


Debugging:

* Define UART_DEBUG in project options
* Attach to UART: P02 <-> TX, P03 <-> RX


Useful links:

* I2C implementation: https://e2e.ti.com/support/wireless-connectivity/zigbee-thread-group/zigbee-and-thread/f/zigbee-thread-forum/140917/can-i-use-i2c-in-cc2530/639420
* VL53l1X datasheet: https://www.st.com/resource/en/datasheet/vl53l1x.pdf
* www.makerguides.com/vl53l1x-tof400c-distance-sensor-with-arduino
* https://github.com/diyruz/flower
* https://github.com/pololu/vl53l1x-arduino
* https://habr.com/ru/companies/iobroker/articles/495926
