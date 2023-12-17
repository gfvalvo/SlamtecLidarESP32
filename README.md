# SlamtecLidarESP32
## Port of Slamtec Lidar SDK for ESP32 in the Arduino Framework
### Version 1.0:
##### This version is based on Version 2.0 of the Slamtec SDK: https://www.slamtec.com/en/Support with the following differences:
* An ESP32 Board with PSRAM is required.
* This library only supports a Serial channel connection to the LIDAR. UDP and TCP channel connenctions are not supported.
* The Serial channel is created by calling the 'createSerialPortChannel' which has the prototype:
```
Result<IChannel*> createSerialPortChannel(Stream &stream);
```
* The Stream object passed by reference to 'createSerialPortChannel' must be initialized in the user code.

##### See the BasicLidarDemo example for usage of the library's functions.
