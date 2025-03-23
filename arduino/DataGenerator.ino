
#include <Arduino_LSM9DS1.h>

float x, y, z;

void setup() {

  Serial.begin(9600);
  while (!Serial);

  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
    
  Serial.println("Time(ms),X,Y,Z");  // CSV Header

}

void loop() {
    
    IMU.readAcceleration(x, y, z);
    
    // Convert raw values to "g"
    float ax = x; 
    float ay = y; 
    float az = z;
    
    // Print CSV-formatted data
    Serial.print(millis());  // Timestamp
    Serial.print(",");
    Serial.print(ax);
    Serial.print(",");
    Serial.print(ay);
    Serial.print(",");
    Serial.println(az);
    
    delay(100);  // Adjust for desired sampling rate
}


