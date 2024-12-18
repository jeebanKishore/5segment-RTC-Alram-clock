#include <FastGPIO.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;
// Macro for I2C address
#define I2C_ADDRESS 0x2A
const byte interruptPin = 2;
volatile byte state = LOW;
int beep_delay = 20;
void sendDataViaI2C(const char* data) {
  Wire.beginTransmission(I2C_ADDRESS); // Start communication with the slave
  Wire.write(data);                    // Send the string
  Wire.endTransmission();             // End communication
  Serial.print("Data sent: ");
  Serial.println(data);
}

void beep() {
  digitalWrite(buzzer_pin, HIGH);
  delay(beep_delay);
  digitalWrite(buzzer_pin, LOW);
}

void setup() {
  FastGPIO::Pin<4>::setInput();
  FastGPIO::Pin<5>::setInput();
  FastGPIO::Pin<6>::setInput();
  FastGPIO::Pin<7>::setInput();  // the number of the pushbutton pin
  FastGPIO::Pin<13>::setOutput(0);
  Wire.begin(); // Join I2C bus as master
  Serial.begin(115200);
  Serial.println("Master ready");
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), beep, CHANGE);
  Serial.println(F("setup(): begin"));
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  
}

void loop() {
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (FastGPIO::Pin<4>::isInputHigh() == HIGH) {
    // turn LED on:
    FastGPIO::Pin<13>::setOutput(1);
  } else {
    // turn LED off:
    FastGPIO::Pin<13>::setOutput(0);
  }

  const char dataToSend[] = "A12340"; // The string to send
  sendDataViaI2C(dataToSend);          // Use the function to send data
  delay(1000);
}
