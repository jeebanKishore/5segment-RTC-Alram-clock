#include <Wire.h>
#include "RTClib.h"

// Global Variables
RTC_DS3231 rtc;                          // Instance of RTC_DS3231 to manage RTC operations.
const byte displayAddress = 0x2A;        // I2C address of the display.
const int buzzerPin = 13;                // Pin number for the buzzer.
volatile boolean isAlarmActive = false;  // Flag to check if the alarm is active.
volatile boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0, sw5_status = 0;
volatile int switch_status = 0;
const int sw1 = 4, sw2 = 5, sw3 = 6, sw4 = 7;
// Debounce Configuration
const int debounceDelay = 50;                                     // Debounce time in milliseconds
unsigned long lastDebounceTime[5] = { 0, 0, 0, 0, 0 };            // Tracks debounce time for each switch
bool lastSwitchState[5] = { false, false, false, false, false };  // Last stable state for each switch

// Function to send data via I2C
void sendDataViaI2C(const char *data) {
  if (strlen(data) != 6) {
    Serial.println("Error: Data must be exactly 6 characters long.");
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
  }
}

// Function to display the current date
void displayDate() {
  DateTime now = rtc.now();
  char dateString[6];
  sprintf(dateString, "d%02d%02d1", now.day(), now.month());
  sendDataRepeatedly(dateString);

  char yearString[6];
  sprintf(yearString, "Y%04d0", now.year());
  sendDataRepeatedly(yearString);
}

// Function to display the alarm time
void displayAlarm(int alarm) {
  char alarmDateString[7];
  DateTime alarmTime = (alarm == 1) ? rtc.getAlarm1() : rtc.getAlarm2();
  int my_hour = alarmTime.hour();
  int my_min = alarmTime.minute();
  char apDigit = (my_hour >= 12) ? 'P' : 'A';
  my_hour = (my_hour > 12) ? my_hour - 12 : my_hour;
  snprintf(alarmDateString, sizeof(alarmDateString), "%c%02d%02d1", apDigit, my_hour, my_min);
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
  DateTime now = rtc.now();
  int my_hour = now.hour();
  int my_min = now.minute();
  const char *apDigit = (my_hour >= 12) ? "P" : "A";
  if (my_hour == 0) my_hour = 12;
  else if (my_hour > 12) my_hour -= 12;

  char dispString[7];  // Increase size to 7
  snprintf(dispString, sizeof(dispString), "%c%02d%02d2", apDigit[0], my_hour, my_min);
  sendDataViaI2C(dispString);
}


// Function to activate the buzzer
void beep(bool continuous = false, int beepDelay = 20) {
  if (continuous) {
    while (isAlarmActive) {
      digitalWrite(buzzerPin, HIGH);
      delay(beepDelay);
      digitalWrite(buzzerPin, LOW);
      delay(beepDelay);
    }
  } else {
    digitalWrite(buzzerPin, HIGH);
    delay(beepDelay);
    digitalWrite(buzzerPin, LOW);
  }
}

// Function to stop the alarm
void stopAlarm() {
  if (!isAlarmActive) return;
  beep();
  // rtc.disableAlarm(1);
  isAlarmActive = false;
  rtc.clearAlarm(1);
}

// Function to activate the alarm
void activateAlarm() {
  isAlarmActive = true;
  beep(true);
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
      displayAlarm(1);
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
    if (setTime()) return 1;
    else if (setDate()) return 1;
    else if (setYear()) return 1;
    else if (setAlarm()) return 1;
  } else {
    beep();
  }
  return 0;
}



// Function to set the current time
int setTime() {
  sendDataRepeatedly("5ttIE0", 2000);
  int time_changed_status = 0;
  int my_hour_tmp = 0;
  DateTime now = rtc.now();
  int my_hour = now.hour();
  int my_min = now.minute();
  beep();
  delay(100);
  readSwitchStatus();

  delay(100);
  while (!sw1_status) {
    readSwitchStatus();
    if (sw2_status) {
      time_changed_status = 1;
      delay(150);
      my_hour++;
      if (my_hour > 23) my_hour = 0;
    } else if (sw3_status) {
      time_changed_status = 1;
      delay(100);
      my_min++;
      if (my_min > 59) my_min = 0;
    } else if (sw4_status) {
      beep();
      delay(100);
      if (time_changed_status) {
        DateTime now = rtc.now();
        int my_date = now.day();
        int my_month = now.month();
        int my_year = now.year();
        rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
        beep();
        delay(50);
        sendDataRepeatedly("TI5Et0", 2000);
        time_changed_status = 0;
      }
      return 1;
    }

    const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";
    if (my_hour == 0) my_hour_tmp = 12;
    else if (my_hour > 12) my_hour_tmp = my_hour - 12;

    char dispString[7];
    snprintf(dispString, sizeof(dispString), "%c%02d%02d1", apDigit[0], my_hour_tmp, my_min);
    sendDataViaI2C(dispString);
  }

  // if (time_changed_status) {
  //   DateTime now = rtc.now();
  //   int my_date = now.day();
  //   int my_month = now.month();
  //   int my_year = now.year();
  //   rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
  //   beep();
  //   delay(50);
  // }


  getTime();
  delay(200);
  return 0;
}

// Function to set the current date
int setDate() {
  sendDataRepeatedly("5tdtE0", 2000);
  int date_changed_status = 0;
  DateTime now = rtc.now();
  int my_date = now.day();
  int my_month = now.month();
  int my_year = now.year();
  readSwitchStatus();
  beep();
  delay(100);
  while (!sw1_status) {
    readSwitchStatus();
    if (sw2_status) {
      date_changed_status = 1;
      delay(150);
      my_date++;
      if (my_date > 31) my_date = 1;
    } else if (sw3_status) {
      date_changed_status = 1;
      delay(150);
      my_month++;
      if (my_month > 12) my_month = 1;
    } else if (sw4_status) {
      beep();
      if (date_changed_status) {
        DateTime now = rtc.now();
        int my_hour = now.hour();
        int my_min = now.minute();
        rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
        beep();
        delay(50);
        date_changed_status = 0;
        sendDataRepeatedly("dt5Et0", 2000);
      }
      return 1;
    }

    char dateString[7];
    sprintf(dateString, "d%02d%02d1", my_date, my_month);
    sendDataViaI2C(dateString);
    delay(5000);

    char yearString[6];
    sprintf(yearString, "Y%04d0", my_year);
    sendDataViaI2C(yearString);
    delay(5000);
  }

  // if (date_changed_status) {
  //   DateTime now = rtc.now();
  //   int my_hour = now.hour();
  //   int my_min = now.minute();
  //   rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
  //   beep();
  //   delay(50);
  // }

  getTime();
  delay(200);
  return 0;
}

// Function to set the current year
int setYear() {
  sendDataRepeatedly("5tYEA0", 2000);
  int year_changed_status = 0;
  DateTime now = rtc.now();
  int my_year = now.year();
  readSwitchStatus();
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    if (sw2_status) {
      year_changed_status = 1;
      delay(150);
      my_year--;
      if (my_year < 2024) my_year = 2024;
    } else if (sw3_status) {
      year_changed_status = 1;
      delay(150);
      my_year++;
      if (my_year > 2070) my_year = 2070;
    } else if (sw4_status) {
      beep();
      if (year_changed_status) {
        DateTime now = rtc.now();
        int my_hour = now.hour();
        int my_min = now.minute();
        int my_date = now.day();
        int my_month = now.month();
        rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
        beep();
        delay(50);
        year_changed_status = 0;
        sendDataRepeatedly("YE5Et", 2000);
      }
      return 1;
    }

    char yearString[6];
    sprintf(yearString, "Y%04d0", my_year);
    sendDataViaI2C(yearString);
  }

  // if (year_changed_status) {
  //   DateTime now = rtc.now();
  //   int my_hour = now.hour();
  //   int my_min = now.minute();
  //   int my_date = now.day();
  //   int my_month = now.month();
  //   rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
  //   beep();
  //   delay(50);
  // }

  getTime();
  delay(200);
  return 0;
}

// Function to set the alarm time
int setAlarm() {
  sendDataRepeatedly("5EtAL0", 2000);
  int alarm_changed_status = 0;
  int alarm_hour = 0, alarm_min = 0, alarm_hour_temp = 0;
  readSwitchStatus();
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    if (sw2_status) {
      alarm_changed_status = 1;
      delay(150);
      alarm_hour++;
      if (alarm_hour > 23) alarm_hour = 0;
    } else if (sw3_status) {
      alarm_changed_status = 1;
      delay(150);
      alarm_min++;
      if (alarm_min > 59) alarm_min = 0;
    } else if (sw4_status) {
      beep();
      if (alarm_changed_status) {
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);
        rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
        beep();
        delay(200);
        alarm_changed_status = 0;
        sendDataRepeatedly("AL5Et0", 2000);
      } else {
        sendDataRepeatedly("ALoFF0", 2000);
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);
      }
      return 1;
    }

    const char *apDigit = (alarm_hour > 12 && alarm_min > 0) ? "P" : "A";
    if (alarm_hour == 0) alarm_hour_temp = 12;
    else if (alarm_hour > 12) alarm_hour_temp = alarm_hour - 12;

    char dispString[7];
    snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], alarm_hour_temp, alarm_min);
    sendDataViaI2C(dispString);
  }

  delay(100);
  getTime();
  // if (alarm_changed_status) {
  //   rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
  //   getTime();
  //   beep();
  //   delay(200);
  // }
  return 0;
}


// Function to read the status of each switch
// boolean readSwitchStatus() {
//   sw5_status = FastGPIO::Pin<3>::isInputHigh() == HIGH;
//   sw1_status = FastGPIO::Pin<4>::isInputHigh() == HIGH;
//   sw2_status = FastGPIO::Pin<5>::isInputHigh() == HIGH;
//   sw3_status = FastGPIO::Pin<6>::isInputHigh() == HIGH;
//   sw4_status = FastGPIO::Pin<7>::isInputHigh() == HIGH;
//   return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);
// }

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
  for (int i = 0; i < 5; i++) {
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
        else if (i == 4) sw5_status = lastSwitchState[i];
      }
    }
  }

  // Return true if any switch is active
  return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);
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
  attachInterrupt(digitalPinToInterrupt(3), stopAlarm, FALLING);
  attachInterrupt(digitalPinToInterrupt(2), activateAlarm, FALLING);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF);
  Serial.println(F("RTC Display Initialized"));
}

// Arduino loop function which runs repeatedly after setup
void loop() {
  Serial.println("Loop Start: ");
  checkSwitch();
  getTime();
  delay(1);
}