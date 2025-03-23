#include <Arduino_LSM9DS1.h>

#include <TensorFlowLite.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "stepmodel.h"

const int numSamples = 20;
int samplesRead = 0;

tflite::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));

const char* GESTURES[] = {"walk", "run"};
#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // initialize the IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  // get the TFL representation of the model byte array
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
    return;
  }

  // Create an interpreter to run the model
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize);

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = tflInterpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  // Get pointers for the model's input and output tensors
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);
}

void loop() {
  float aX, aY, aZ;

  // normalize the IMU data between 0 to 1 and store in the model's input tensor
  if (samplesRead < numSamples && IMU.accelerationAvailable()) {
    IMU.readAcceleration(aX, aY, aZ);
    tflInputTensor->data.f[samplesRead * 3 + 0] = aX;
    tflInputTensor->data.f[samplesRead * 3 + 1] = aY;
    tflInputTensor->data.f[samplesRead * 3 + 2] = aZ;
    samplesRead++;
  }
	
  // Run inferencing
  if (samplesRead == numSamples) {
    TfLiteStatus invokeStatus = tflInterpreter->Invoke();
    if (invokeStatus != kTfLiteOk) {
      Serial.println("Invoke failed!");
      while (1);
      return;
    }

    // Loop through the output tensor values from the model
    for (int i = 0; i < NUM_GESTURES; i++) {
      Serial.print(GESTURES[i]);
      Serial.print(": ");
      Serial.println(tflOutputTensor->data.f[i], 6);
    }

    if (tflOutputTensor->data.f[0] < 0.6) {
      Serial.println("running");
    } else {
      Serial.println("walking");
    }

    // Clean up the data buffer before filling up for the next batch.
    int i = 0;
    for (; i< numSamples; i ++) {
      tflInputTensor->data.f[i * 3 + 0];
    }

    Serial.println();
    samplesRead = 0;
    delay(100);
  }
}
