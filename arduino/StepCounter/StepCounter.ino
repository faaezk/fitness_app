//#include <Wire.h>
//#include <MPU6050.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoBLE.h>

//MPU6050 mpu;
float x, y, z;
int stepCount = 0;
float prevAccelZ = 0;
float threshold = 0.01;

// Create Service
BLEService stepService("19B10010-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic stepCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);


void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  Serial.print("Accelerometer sample rate = ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");
  Serial.println();
  Serial.println("Acceleration in g's");
  Serial.println("X\tY\tZ");

  // Start BLE
  if (!BLE.begin()) {
    Serial.println("Failed to start BLE!");
    while (1);
  }

  BLE.setLocalName("StepCounter");
  BLE.setAdvertisedService(stepService);

  // Add characteristic to service
  stepService.addCharacteristic(stepCharacteristic);
  BLE.addService(stepService);

  BLE.advertise();
  Serial.println("BLE Device Ready");

}

void loop() {

  // Check if a device is connected
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      IMU.readAcceleration(x, y, z);
      float accelz = z/100.0;
      //Serial.println(accelz);

      if (prevAccelZ < threshold && accelz >= threshold){
        stepCount++;
        Serial.print("Step Count: ");
        Serial.println(stepCount);
        stepCharacteristic.writeValue(stepCount);
      }

      prevAccelZ = accelz;
      delay(100);
    }  

  }  

}
