#include <FastGPIO.h>
#include <Wire.h>
#include "RTClib.h"

// Global Variables
RTC_DS3231 rtc;                    // Instance of RTC_DS3231 to manage RTC operations.
const byte displayAddress = 0x2A;  // I2C address of the display.
const int buzzerPin = 13;          // Pin number for the buzzer.
boolean isAlarmActive = false;     // Flag to check if the alarm is active.
boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0, sw5_status = 0;
int switch_status = 0;
// Function to send data via I2C
void sendDataViaI2C(const char *data) {
  Serial.print("Data sent: ");
  Serial.println(data);
  if (strlen(data) != 6) {
    Serial.println("Error: Data must be exactly 6 characters long.");
    return;
  }
  // Wire.beginTransmission(displayAddress);
  // Wire.write(data);
  // Wire.endTransmission();
  Serial.print("Data sent: ");
  Serial.println(data);
}

// Function to send data repeatedly for a specified duration
void sendDataRepeatedly(const char *data) {
  long startTime = millis();
  while ((millis() - startTime) <= 5000) {
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
      menu();  // go to menu if switch one is pressed
      delay(200);
    }

    else if (sw2_status) {
      displayDate();  // display date if switch 2 is pressed
    }

    else if (sw3_status) {
      displayTemperature();  // display tempreature if switch 3 is pressed
    }
    else if (sw4_status) {
      displayAlarm(1);
    }
  }
}

// Function to display the menu
void displayMenu() {
  Serial.println("Entered menu");
  delay(100);
  while (!anySwitchActive()) {
    sendDataViaI2C("Enu==1");
    DateTime now = rtc.now();
    if (now.second() % 2) {
      sendDataViaI2C("-----1");
    } else {
      sendDataViaI2C("Enu==1");
    }
  }

  if (sw1_status) {
    beep();
    delay(300);
    menu();
    delay(200);
  } else {
    beep();
  }
}

// Function to manage the menu options
void menu() {
  Serial.println("Entered menu");
  delay(100);
  while (!anySwitchActive()) {
    sendDataViaI2C("Enu==1");
    DateTime now = rtc.now();
    if (now.second() % 2) {
      sendDataViaI2C("-----1");
    } else {
      sendDataViaI2C("Enu==1");
    }
  }
delay(3000);
  if (sw1_status) {
    delay(100);
    if (setTime()) return;
    else if (setDate()) return;
    else if (setYear()) return;
    else if (setAlarm()) return;
  } else {
    beep();
  }
}

// Function to set the current time
int setTime() {
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
      return 1;
    }

    const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";
    if (my_hour == 0) my_hour_tmp = 12;
    else if (my_hour > 12) my_hour_tmp = my_hour - 12;

    char dispString[7];
    snprintf(dispString, sizeof(dispString), "%c%02d%02d1", apDigit[0], my_hour_tmp, my_min);
    sendDataViaI2C(dispString);
  }

  delay(100);
  if (time_changed_status) {
    DateTime now = rtc.now();
    int my_date = now.day();
    int my_month = now.month();
    int my_year = now.year();
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }

  getTime();
  delay(200);
  return 0;
}

// Function to set the current date
int setDate() {
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

  if (date_changed_status) {
    DateTime now = rtc.now();
    int my_hour = now.hour();
    int my_min = now.minute();
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }

  getTime();
  delay(200);
  return 0;
}

// Function to set the current year
int setYear() {
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
      return 1;
    }

    char yearString[6];
    sprintf(yearString, "Y%04d0", my_year);
    sendDataViaI2C(yearString);
  }

  if (year_changed_status) {
    DateTime now = rtc.now();
    int my_hour = now.hour();
    int my_min = now.minute();
    int my_date = now.day();
    int my_month = now.month();
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }

  getTime();
  delay(200);
  return 0;
}

// Function to set the alarm time
int setAlarm() {
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
  rtc.disableAlarm(1);
  rtc.clearAlarm(1);
  if (alarm_changed_status) {
    rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
    getTime();
    beep();
    delay(200);
  }
  return 0;
}

// Function to read the status of each switch
boolean readSwitchStatus() {
  sw5_status = FastGPIO::Pin<3>::isInputHigh() == HIGH;
  sw1_status = FastGPIO::Pin<4>::isInputHigh() == HIGH;
  sw2_status = FastGPIO::Pin<5>::isInputHigh() == HIGH;
  sw3_status = FastGPIO::Pin<6>::isInputHigh() == HIGH;
  sw4_status = FastGPIO::Pin<7>::isInputHigh() == HIGH;
  return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);
}

// Function to check if any switch is active
boolean anySwitchActive() {
  return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);
}

// Arduino setup function which runs once at the start
void setup() {
  Serial.begin(115200);
  FastGPIO::Pin<4>::setInput();
  FastGPIO::Pin<5>::setInput();
  FastGPIO::Pin<6>::setInput();
  FastGPIO::Pin<7>::setInput();
  FastGPIO::Pin<13>::setOutput(0);
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
  Serial.print("Loop Start: ");
  checkSwitch();
  getTime();
}