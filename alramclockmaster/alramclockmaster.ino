#include <Wire.h>
#include "RTClib.h"
#define SNOOZE_DURATION 300000  // Snooze duration in milliseconds (e.g., 5 minutes)
// Global Variables
RTC_DS3231 rtc;                             // Instance of RTC_DS3231 to manage RTC operations.
const byte displayAddress = 0x2A;           // I2C address of the display.
const uint8_t buzzerPin = 13;               // Pin number for the buzzer.
volatile boolean isAlarmActive = false;     // Flag to check if the alarm is active.
volatile unsigned long alarmStartTime = 0;  // When the alram started from
volatile boolean alarmTriggered = false;    // New flag for alarm activation
volatile boolean snoozeflag = 0;
volatile boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0;
volatile uint8_t switch_status = 0;
const uint8_t sw1 = 4, sw2 = 5, sw3 = 6, sw4 = 7;
// Debounce Configuration
const uint8_t debounceDelay = 50;                                 // Debounce time in milliseconds
unsigned long lastDebounceTime[5] = { 0, 0, 0, 0, 0 };            // Tracks debounce time for each switch
bool lastSwitchState[5] = { false, false, false, false, false };  // Last stable state for each switch
uint16_t my_year = 0;
uint8_t my_month = 0, my_date = 0, my_hour = 0, my_min = 0, my_sec = 0, alarm_hour = 0, alarm_min = 0;

unsigned long lastBeepTime = 0;
uint16_t currentBeepDelay = 1500;
bool beepState = false;

// Updaate Gloal time details
int updateGlobalTimeVars() {
  DateTime now = rtc.now();
  my_hour = now.hour();
  my_min = now.minute();
  my_sec = now.second();
  my_date = now.day();
  my_month = now.month();
  my_year = now.year();
  DateTime alram1 = rtc.getAlarm1();
  alarm_hour = alram1.hour();
  alarm_min = alram1.minute();
  return 0;
}
// Function to send data via I2C
void sendDataViaI2C(const char *data) {
  if (strlen(data) != 6) {
    Serial.println("Error: Data must be exactly 6 characters long.");
    Serial.println(data);
    return;
  }
  Wire.beginTransmission(displayAddress);
  Wire.write(data);
  Wire.endTransmission();
}

// Function to send data repeatedly for a specified duration
void sendDataRepeatedly(const char *data, unsigned int timeFrame = 5000) {
  long startTime = millis();
  while ((millis() - startTime) <= timeFrame) {
    sendDataViaI2C(data);
    delay(1);
  }
}

// Function to display the current date
void displayDate() {
  char dateString[6];
  sprintf(dateString, "d%02d%02d1", my_date, my_month);
  sendDataRepeatedly(dateString);

  char yearString[6];
  sprintf(yearString, "Y%04d0", my_year);
  sendDataRepeatedly(yearString);
}

// Function to display the alarm time
void displayAlarm() {
  char alarmDateString[7];
  char apDigit = (alarm_hour >= 12) ? 'P' : 'A';
  uint8_t alarm_hour_tmp = (alarm_hour == 0) ? 12 : (alarm_hour > 12) ? alarm_hour - 12
                                                                      : alarm_hour;
  snprintf(alarmDateString, sizeof(alarmDateString), "%c%02d%02d1", apDigit, alarm_hour_tmp, alarm_min);
  sendDataRepeatedly(alarmDateString);
}

// Function to display the current temperature
void displayTemperature() {
  char dispString[6];
  int tempInt = static_cast<int>(rtc.getTemperature() * 100);
  sprintf(dispString, "t%04d1", tempInt);
  sendDataRepeatedly(dispString);
}

// Function to get and display the current time
void getTime() {
  const char *apDigit = (my_hour >= 12) ? "P" : "A";
  uint8_t my_hour_tmp = (my_hour == 0) ? 12 : (my_hour > 12) ? my_hour - 12
                                                             : my_hour;

  char dispString[7];  // Increase size to 7
  snprintf(dispString, sizeof(dispString), "%c%02d%02d2", apDigit[0], my_hour_tmp, my_min);
  sendDataViaI2C(dispString);
}


void beep(bool continuous = false, int initialBeepDelay = 20) {
  if (continuous) {
    if (isAlarmActive) {
      unsigned long currentTime = millis();
      if (currentTime - lastBeepTime >= currentBeepDelay) {
        beepState = !beepState;  // Toggle beep state
        digitalWrite(buzzerPin, beepState ? HIGH : LOW);
        lastBeepTime = currentTime;

        // Gradually decrease the delay, but not below 300ms
        if (beepState && currentBeepDelay > 300) {
          currentBeepDelay -= 50;                              // Decrease the delay by 50ms each iteration
          if (currentBeepDelay < 300) currentBeepDelay = 300;  // Cap at 300ms
        }
      }
    } else {
      digitalWrite(buzzerPin, LOW);  // Turn off beep when alarm is not active
    }
  } else {
    digitalWrite(buzzerPin, HIGH);
    delay(initialBeepDelay);
    digitalWrite(buzzerPin, LOW);
  }
}


// Function to stop the alarm completely
int stopAlarm() {
  Serial.println("RTC Alarm stopping....");
  if (!isAlarmActive) return;
  Serial.println("Alarm stopped");
  isAlarmActive = false;
  snoozeflag = false;
  rtc.clearAlarm(1);                   // Clear any active alarm flags
  sendDataRepeatedly("ALoFF0", 5000);  // Notify the display
  beep();                              // Signal that the alarm is stopped
  return 0;
}

// Function to snooze the alarm for 5 minutes
int snoozeAlarm() {
  if (!isAlarmActive) return;
  Serial.println("Alarm snoozed for 5 minutes");
  isAlarmActive = false;                        // Temporarily disable the alarm
  alarmStartTime = millis() + SNOOZE_DURATION;  // Set snooze duration
  rtc.clearAlarm(1);                            // Clear the alarm
  sendDataRepeatedly("SnooZ0", 2000);           // Notify the display
  beep();                                       // Signal snooze activation
  return 0;
}

// Function to activate the alarm
void activateAlarm() {
  Serial.println("Alram triggred");
  alarmTriggered = true;  // Set the flag for alarm activation
}

void checkSwitch() {
  switch_status = readSwitchStatus();
  if (switch_status) {
    if (sw1_status) {
      beep();
      delay(300);
      displayMenu();  // go to menu if switch one is pressed
      delay(200);
    } else if (sw2_status) {
      beep();
      displayDate();  // display date if switch 2 is pressed
    } else if (sw3_status) {
      beep();
      displayTemperature();  // display tempreature if switch 3 is pressed
    } else if (sw4_status) {
      beep();
      displayAlarm();
    }
  }
}

// Function to display the menu
int displayMenu() {
  Serial.println("Entered menu");
  sendDataRepeatedly("Enu==1", 2000);
  delay(100);
  // display an animation when entered into menu
  while (!readSwitchStatus()) {
    DateTime now = rtc.now();
    if (now.second() % 2) {
      sendDataRepeatedly("-----1", 2000);
    } else {
      sendDataRepeatedly("Enu==1", 2000);
    }
  }

  if (sw1_status) {
    // Set the clock parameters
    delay(100);
    if (setAlarm()) return 1;
    else if (setTime()) return 1;
    else if (setDate()) return 1;
    else if (setYear()) return 1;
  } else {
    beep();
  }
  return 0;
}



// Function to set the current time
int setTime() {
  sendDataRepeatedly("5ttIE0", 2000);
  boolean time_changed_status = 0;
  updateGlobalTimeVars();
  beep();
  delay(100);
  readSwitchStatus();
  while (!sw1_status) {
    readSwitchStatus();
    sendUpdatedTimeOrAlaramToDisplay(my_hour, my_min);
    if (sw2_status) {
      time_changed_status = 1;
      delay(150);
      my_hour++;
      if (my_hour > 23) my_hour = 0;
      sendUpdatedTimeOrAlaramToDisplay(my_hour, my_min);
    } else if (sw3_status) {
      time_changed_status = 1;
      delay(150);
      my_min++;
      if (my_min > 59) my_min = 0;
      sendUpdatedTimeOrAlaramToDisplay(my_hour, my_min);
    } else if (sw4_status) {
      beep();
      delay(100);
      if (time_changed_status) {
        rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
        beep();
        delay(50);
        sendDataRepeatedly("tI5Et0", 2000);
        time_changed_status = 0;
        sendUpdatedTimeOrAlaramToDisplay(my_hour, my_min);
      }
      return 1;
    }
  }
  updateGlobalTimeVars();
  delay(200);
  return 0;
}

int sendUpdatedTimeOrAlaramToDisplay(uint8_t my_hour, uint8_t my_min) {
  uint8_t my_hour_tmp = (my_hour == 0) ? 12 : (my_hour > 12) ? my_hour - 12
                                                             : my_hour;
  const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";
  char dispString[7];
  snprintf(dispString, sizeof(dispString), "%c%02d%02d1", apDigit[0], my_hour_tmp, my_min);
  sendDataViaI2C(dispString);
  return 0;
}

// Function to set the current date
int setDate() {
  sendDataRepeatedly("5tdtE0", 5000);
  boolean date_changed_status = 0;
  updateGlobalTimeVars();
  readSwitchStatus();
  beep();
  delay(100);
  while (!sw1_status) {
    readSwitchStatus();
    sendUpdatedDateToDisplay(my_date, my_month);
    if (sw2_status) {
      date_changed_status = 1;
      delay(150);
      my_date++;
      if (my_date > 31) my_date = 1;
      sendUpdatedDateToDisplay(my_date, my_month);
    } else if (sw3_status) {
      date_changed_status = 1;
      delay(150);
      my_month++;
      if (my_month > 12) my_month = 1;
      sendUpdatedDateToDisplay(my_date, my_month);
    } else if (sw4_status) {
      beep();
      if (date_changed_status) {
        sendUpdatedDateToDisplay(my_date, my_month);
        DateTime now = rtc.now();
        uint8_t my_hour = now.hour();
        uint8_t my_min = now.minute();
        rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
        beep();
        delay(50);
        date_changed_status = 0;
        sendDataRepeatedly("dt5Et0", 2000);
      }
      return 1;
    }
  }
  updateGlobalTimeVars();
  delay(200);
  return 0;
}

int sendUpdatedDateToDisplay(uint8_t my_date, uint8_t my_month) {
  char dateString[7];
  sprintf(dateString, "d%02d%02d1", my_date, my_month);
  sendDataViaI2C(dateString);
  return 0;
}

// Function to set the current year
int setYear() {
  sendDataRepeatedly("5tYEA0", 5000);
  boolean year_changed_status = 0;
  updateGlobalTimeVars();
  Serial.println("Year set start");
  Serial.println(my_year);
  readSwitchStatus();
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    setUpdatedYearToDisplay(my_year);
    if (sw2_status) {
      year_changed_status = 1;
      delay(150);
      my_year--;
      if (my_year < 2000) my_year = 2000;
      setUpdatedYearToDisplay(my_year);
    } else if (sw3_status) {
      year_changed_status = 1;
      delay(150);
      my_year++;
      if (my_year > 2070) my_year = 2070;
      setUpdatedYearToDisplay(my_year);
    } else if (sw4_status) {
      beep();
      setUpdatedYearToDisplay(my_year);
      if (year_changed_status) {
        Serial.println("adter button press");

        Serial.println(my_year);
        Serial.println(my_month);
        Serial.println(my_date);
        Serial.println(my_hour);
        Serial.println(my_min);
        Serial.println("****************");
        rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
        beep();
        delay(50);

        sendDataRepeatedly("YE5Et0", 5000);
        year_changed_status = 0;
        setUpdatedYearToDisplay(my_year);
      }
      return 1;
    }
  }



  updateGlobalTimeVars();
  delay(200);
  return 0;
}

int setUpdatedYearToDisplay(uint16_t my_year) {
  char yearString[6];
  sprintf(yearString, "Y%04d0", my_year);
  sendDataViaI2C(yearString);
  return 0;
}

// Function to set the alarm time
int setAlarm() {
  sendDataRepeatedly("5EtAL0", 2000);
  updateGlobalTimeVars();
  Serial.println(alarm_hour);
  Serial.println(alarm_min);
  boolean alarm_changed_status = 0;
  readSwitchStatus();
  // sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
    if (sw2_status) {
      alarm_changed_status = 1;
      delay(150);
      alarm_hour++;
      if (alarm_hour > 23) alarm_hour = 0;
      sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
    } else if (sw3_status) {
      alarm_changed_status = 1;
      delay(150);
      alarm_min++;
      if (alarm_min > 59) alarm_min = 0;
      sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
    } else if (sw4_status) {
      beep();
      if (alarm_changed_status) {
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);
        rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
        beep();
        delay(200);
        sendDataRepeatedly("AL5Et0", 2000);
        alarm_changed_status = 0;
        sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
        Serial.println("After Set alram");
        Serial.println(alarm_hour);
        Serial.println(alarm_min);
      } else {
        sendDataRepeatedly("ALoFF0", 2000);
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);
      }
      return 1;
    }
  }

  delay(100);
  updateGlobalTimeVars();
  return 0;
}

//Function to read the status of each switch with debounce
boolean readSwitchStatus() {
  unsigned long currentTime = millis();

  // Read raw switch inputs
  bool rawSwitchStates[5] = {
    sw1_status = digitalRead(sw1),
    sw2_status = digitalRead(sw2),
    sw3_status = digitalRead(sw3),
    sw4_status = digitalRead(sw4),
  };

  // Apply debounce logic
  for (uint8_t i = 0; i < 5; i++) {
    if (rawSwitchStates[i] != lastSwitchState[i]) {
      // Switch state has changed, reset debounce timer
      lastDebounceTime[i] = currentTime;
    }

    if ((currentTime - lastDebounceTime[i]) > debounceDelay) {
      // If stable for debounce period, update the stable state
      if (rawSwitchStates[i] != lastSwitchState[i]) {
        lastSwitchState[i] = rawSwitchStates[i];

        // Update the respective switch status
        if (i == 0) sw1_status = lastSwitchState[i];
        else if (i == 1) sw2_status = lastSwitchState[i];
        else if (i == 2) sw3_status = lastSwitchState[i];
        else if (i == 3) sw4_status = lastSwitchState[i];
      }
    }
  }

  // Return true if any switch is active
  return (sw1_status || sw2_status || sw3_status || sw4_status);
}

// Arduino setup function which runs once at the start
void setup() {
  Serial.begin(115200);
  pinMode(sw1, INPUT);
  pinMode(sw2, INPUT);
  pinMode(sw3, INPUT);
  pinMode(sw4, INPUT);

  pinMode(buzzerPin, OUTPUT);
  Wire.begin();
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), activateAlarm, FALLING);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  rtc.disable32K();
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);

  rtc.writeSqwPinMode(DS3231_OFF);
  Serial.println(F("RTC Display Initialized"));
}


void loop() {
  updateGlobalTimeVars();
  checkSwitch();
  getTime();

  // Handle the interrupt flag in the main loop
  if (alarmTriggered) {
    alarmTriggered = false;  // Reset the interrupt flag

    if (rtc.alarmFired(1)) {  // Check if the alarm is triggered
      Serial.println(F("RTC Alarm triggered"));
      isAlarmActive = true;
      alarmStartTime = millis();  // Record the alarm start time
      currentBeepDelay = 20;      // Reset beep delay
    } else {
      Serial.println(F("False interrupt detected"));
    }
  }

  // Handle active alarm
  if (isAlarmActive) {
    rtc.clearAlarm(1);  // Clear the alarm

    // Check switch inputs
    if (digitalRead(sw4)) {
      // Stop the alarm
      Serial.println(F("RTC Alarm stopping"));
      stopAlarm();
      return;
    } else if (digitalRead(sw1) || digitalRead(sw2) || digitalRead(sw3)) {
      // Snooze the alarm for 5 minutes
      snoozeflag = true;
      snoozeAlarm();
      return;
    }

    // Check if 15 minutes have elapsed since the alarm started
    if (millis() - alarmStartTime >= 15 * 60 * 1000) {
      Serial.println(F("Alarm auto-stopped after 15 minutes"));
      stopAlarm();
      return;
    }

    // Continue alarm sound
    beep(true, currentBeepDelay);  // Ensure beep is non-blocking
  }

  // Handle snoozed alarm
  if (!isAlarmActive && snoozeflag && millis() >= alarmStartTime) {
    Serial.println(F("Snoozed alarm reactivated"));
    isAlarmActive = true;
    alarmStartTime = millis();  // Reset the alarm start time
  }

  delay(1);
}