#include <Arduino_LSM9DS1.h>
#include <ArduinoBLE.h>
#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "stepmodel.h"

int stepCount = 0;
float prevAccelz = 0;
int samplesRead = 0;
int activity = 0;
int lastActivity = -1;
int lastLastActivity = -2;

tflite::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

constexpr int tensorArenaSize = 2 * 1024;
byte tensorArena[tensorArenaSize] __attribute__((aligned(4)));

// BLE Service and Characteristics
BLEService stepService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic stepCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);
BLEByteCharacteristic activityCharacteristic("19b10012-e8f2-537e-4f6c-d104768a1214", BLERead | BLENotify);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Failed to initialize BLE!");
    while (1);
  }

  BLE.setLocalName("StepTracker");
  BLE.setAdvertisedService(stepService);
  stepService.addCharacteristic(stepCharacteristic);
  stepService.addCharacteristic(activityCharacteristic);
  BLE.addService(stepService);
  BLE.advertise();
  Serial.println("BLE Started");

  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // Load the TFL model
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    return;
  }

  // Create interpreter
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize);
  
  // Allocate tensors
  if (tflInterpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    return;
  }

  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    float x, y, z;
    
    IMU.readAcceleration(x, y, z);
    float accelz = z / 100.0;
    if (prevAccelz < 0.01 && accelz >= 0.01) {
      stepCount++;
      stepCharacteristic.writeValue(stepCount);
    }
    prevAccelz = accelz;

    // Collect IMU data for inference
    if (samplesRead < 20) {
      tflInputTensor->data.f[samplesRead * 3 + 0] = x;
      tflInputTensor->data.f[samplesRead * 3 + 1] = y;
      tflInputTensor->data.f[samplesRead * 3 + 2] = z;
      samplesRead++;
    }
        
    // Run inferencing
    if (samplesRead == 20) {
      if (tflInterpreter->Invoke() == kTfLiteOk) {
        lastLastActivity = lastActivity;
        lastActivity = activity;
        activity = tflOutputTensor->data.f[0] < 0.6 ? 0 : 1; // idle = 0, moving = 1
        if (activity == lastActivity && activity == lastLastActivity) {
          activityCharacteristic.writeValue(activity);
        }
      } else {
        Serial.println("Invoke failed!");
      }

      // Reset input tensor data for next round
      for (int i = 0; i < tflInputTensor->bytes / sizeof(float); i++) {
        tflInputTensor->data.f[i] = 0.0f;
      }

      samplesRead = 0;
      delay(50);
    }
  }
}
