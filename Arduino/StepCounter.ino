//#include <Wire.h>
//#include <MPU6050.h>
#include <Arduino_LSM9DS1.h>
#include <ArduinoBLE.h>

//MPU6050 mpu;
float x, y, z;
int stepCount = 0;
float prevAccelZ = 0;
float threshold = 0.01;

BLEService stepService("19B10010-E8F2-537E-4F6C-D104768A1214"); // create service

// create switch characteristic and allow remote device to read and write
BLEByteCharacteristic stepCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | 
BLEWrite | BLENotify);


void setup() {
  // put your setup code here, to run once:
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

  BLEDevice central = BLE.central(); // Check if a device is connected

  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      // put your main code here, to run repeatedly:
      IMU.readAcceleration(x, y, z);
      float accelz = z/100.0;
      //Serial.println(accelz);

      if (prevAccelZ < threshold && accelz >= threshold){
        stepCount++;
        Serial.print("Step Count: ");
        Serial.println(stepCount);
      }

      prevAccelZ = accelz;
      delay(100);
    }  

  }  

}
