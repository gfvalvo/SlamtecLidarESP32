#include "Arduino.h"

#include "SlamtecLidarESP32.h"
#include <algorithm>

void receiveScan(void *params);

const uint8_t motorControlPin = 14;

using namespace sl;

bool checkSLAMTECLIDARHealth(ILidarDriver *drv);

void setup() {
  const uint8_t rxPin = 27;
  const uint8_t txPin = 33;

  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, rxPin, txPin);
  pinMode(motorControlPin, OUTPUT);
  digitalWrite(motorControlPin, LOW);
  delay(3000);

  Serial.printf("Ultra simple LIDAR data grabber for SLAMTEC LIDAR.\n"
                "Version: %d.%d.%d\n\n", SL_LIDAR_SDK_VERSION_MAJOR, SL_LIDAR_SDK_VERSION_MINOR, SL_LIDAR_SDK_VERSION_PATCH);
  sl_result op_result;

  IChannel *_channel;
  ILidarDriver *drv = *createLidarDriver();
  assert(drv != nullptr);

  sl_lidar_response_device_info_t devinfo;
  _channel = *createSerialPortChannel(Serial2);

  bool connectSuccess = false;
  if (SL_IS_OK((drv)->connect(_channel))) {
    op_result = drv->getDeviceInfo(devinfo);
    if (SL_IS_OK(op_result)) {
      connectSuccess = true;
    }
    else {
      log_e("Failed to get Device Info");
      delete drv;
      drv = nullptr;
    }
  } else {
    log_e("Connect Failed");
  }

  if (connectSuccess) {
    Serial.printf("SLAMTEC LIDAR S/N: ");
    for (int pos = 0; pos < 16; ++pos) {
      Serial.printf("%02X", devinfo.serialnum[pos]);
    }

    Serial.printf("\n"
                  "Firmware Ver: %d.%02d\n"
                  "Hardware Rev: %d\n"
                  , devinfo.firmware_version >> 8
                  , devinfo.firmware_version & 0xFF
                  , (int) devinfo.hardware_version);

    if (!checkSLAMTECLIDARHealth(drv)) {
      while (1)
        ;
    }
    BaseType_t returnCode = xTaskCreatePinnedToCore(receiveScan, "receiveScan", 3200, drv, 6, NULL, CONFIG_ARDUINO_RUNNING_CORE);
    assert(returnCode != pdFAIL);
  }
}

void loop() {
}

void receiveScan(void *params) {
  const size_t maxNodes = 8192;
  ILidarDriver *drv = reinterpret_cast<ILidarDriver*>(params);
  sl_lidar_response_measurement_node_hq_t *nodes = reinterpret_cast<sl_lidar_response_measurement_node_hq_t*>(ps_malloc(maxNodes * sizeof(sl_lidar_response_measurement_node_hq_t)));
  assert(nodes != nullptr);
  size_t count = maxNodes;
  sl_result op_result;
  std::vector<LidarScanMode> modes;

  Result<nullptr_t> ans = drv->getAllSupportedScanModes(modes);
  if (!ans) {
    log_e("No Scan Modes Detected");
    free(nodes);
    modes.clear();
    vTaskDelete(NULL);
  }

  while (1) {
    Serial.printf("Avaiable Scan Modes:\n\n");
    for (auto &mode : modes) {
      Serial.printf("Mode ID: %d, Mode Name: %s, ", mode.id, mode.scan_mode);
      Serial.printf("uS Per Sample: %.2f, Max Distance: %.2f, Answer Type: 0x%.2X\n", mode.us_per_sample, mode.max_distance, mode.ans_type);
    }

    Serial.printf("\nEnter # for Desired Scan Type (press any key to stop scan after it starts): ");
    std::vector<LidarScanMode>::iterator selectedMode;
    int selection;
    while (1) {

      if ((selection = Serial.read()) >= '0') {
        selection -= '0';
        auto predicate = [selection](LidarScanMode & mode) {
          return (selection == mode.id);
        };
        selectedMode = std::find_if(modes.begin(), modes.end(), predicate);
        if (selectedMode != modes.end()) {
          break;
        }
      }
    }

    Serial.printf("\nStarting Scan Mode: %s\n", selectedMode->scan_mode);
    while (Serial.read() >= 0) {
    }

    digitalWrite(motorControlPin, HIGH);
    vTaskDelay(500);
    drv->startScanExpress(false, selectedMode->id, 0, nullptr);

    while (1) {
      op_result = drv->grabScanDataHq(nodes, count);
      if (SL_IS_OK(op_result)) {
        drv->ascendScanData(nodes, count);
        for (int pos = 0; pos < (int) count; ++pos) {
          Serial.printf("%s theta: %03.2f Dist: %08.2f Q: %d \n",
                        (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ? "S " : "  ",
                        (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                        nodes[pos].dist_mm_q2 / 4.0f,
                        nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
        }
      }

      if (Serial.available()) {
        drv->stop();
        digitalWrite(motorControlPin, LOW);
        Serial.println("Scanning Stopped\n\n");
        while (Serial.read() >= 0) {
        }
        break;
      }
    }
  }
}

bool checkSLAMTECLIDARHealth(ILidarDriver *drv) {
  sl_lidar_response_device_health_t healthinfo;
  std::vector<LidarScanMode> modes;
  Result<nullptr_t> ans = SL_RESULT_OK;
  MotorCtrlSupport motorCtrlSupport;

  Serial.printf("Resetting\n");
  drv->reset();
  delay(2000);
  ans = drv->getHealth(healthinfo);
  if (ans) {
    Serial.printf("SLAMTEC Lidar health status : %d\n", healthinfo.status);
    if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
      log_e("Error, slamtec lidar internal error detected. Please reboot the device to retry.");
      return false;
    }
  } else {
    log_e("Error, cannot retrieve the lidar health code: %x", ans);
    return false;
  }

  ans = drv->checkMotorCtrlSupport(motorCtrlSupport);
  if (ans) {
    Serial.printf("Motor Speed Control Support: ");
    switch (motorCtrlSupport) {
      case MotorCtrlSupportNone:
        Serial.printf("None\n");
        break;

      case MotorCtrlSupportPwm:
        Serial.printf("PWM\n");
        break;

      case MotorCtrlSupportRpm:
        Serial.printf("RPM\n");
        break;

      default:
        break;
    }
  } else {
    log_e("Couldn't Get Motor Speed Control Info");
  }

  Serial.printf("\n");

  return true;
}
