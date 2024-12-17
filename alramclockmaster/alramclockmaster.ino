#include <Wire.h>

void setup() {
  Wire.begin(); // Join I2C bus as master
  Serial.begin(115200);
  Serial.println("Master ready");
}

void loop() {
  const char dataToSend[] = "A12342"; // The string to send
  Wire.beginTransmission(0x2A);       // Start communication with the slave (address 42)
  Wire.write(dataToSend);             // Send the string
  Wire.endTransmission();             // End communication
  Serial.println("Data sent: A12340");

  delay(1000); // Delay for 1 second before sending again
}
