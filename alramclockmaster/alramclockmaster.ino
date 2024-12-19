#include <FastGPIO.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;
// Macro for I2C address
#define DISPLAY_ADDRESS 0x2A
const byte interruptPin = 2;
const byte alarmStopPin = 3;
const int buzzer_pin = 13;
volatile byte state = LOW;
int beep_delay = 20;
boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0, sw5_status = 0, switch_status = 0;
boolean isAlarmActive = false;

void sendDataViaI2C(const char *data) {
  // Validate the input data length
  if (strlen(data) != 6) {
    Serial.println("Error: Data must be exactly 6 characters long.");
    return;
  }

  // Send the data directly over I2C
  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write(data);  // Directly write the input string
  Wire.endTransmission();

  // Print the sent data for debugging
  Serial.print("Data sent: ");
  Serial.println(data);
}


void displayTemperature() {
  char dispString[6];


  int tempInt = static_cast<int>(rtc.getTemperature() * 100);


  sprintf(dispString, "t%04d0", tempInt);

  long t = millis();
  while ((millis() - t) <= 5000) {
    sendDataViaI2C(dispString);
  }
  return;
}

void displayAlarm(int alarm) {
  char alarmDateString[6];
  DateTime alarmTime = (alarm == 1) ? rtc.getAlarm1() : rtc.getAlarm2();
  int my_hour = alarmTime.hour();
  int my_min = alarmTime.minute();

  char apDigit = (my_hour >= 12) ? 'P' : 'A';
  my_hour = (my_hour > 12) ? my_hour - 12 : my_hour;


  snprintf(alarmDateString, sizeof(alarmDateString), "%c%02d%02d1", apDigit, my_hour, my_min);


  long startTime = millis();
  while ((millis() - startTime) <= 5000) {
    sendDataViaI2C(alarmDateString);
  }
  return;
}

void displayDate() {
  long t = 0;
  DateTime now = rtc.now();
  int my_date = now.day();
  int my_month = now.month();
  int my_year = now.year();


  char dateString[6];


  sprintf(dateString, "d%02d%02d", my_date, my_month);


  t = millis();
  while ((millis() - t) <= 5000) {
    sendDataViaI2C(dateString, "1");
  }


  char yearString[6];


  sprintf(yearString, "Y%04d", my_year);


  t = millis();
  while ((millis() - t) <= 5000) {
    sendDataViaI2C(yearString, "0");
  }
  return;
}

void getTime() {
  DateTime now = rtc.now();
  int my_hour = now.hour();
  int my_min = now.minute();
  const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";

  if (my_hour == 0) {
    my_hour = 12;
  } else if (my_hour > 12) {
    my_hour -= 12;
  }

  char dispString[6];
  snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], my_hour, my_min);
  sendDataViaI2C(dispString, "0");
}

boolean read_switch_status() {
  sw5_status = FastGPIO::Pin<3>::isInputHigh() == HIGH;
  sw1_status = FastGPIO::Pin<4>::isInputHigh() == HIGH;
  sw2_status = FastGPIO::Pin<5>::isInputHigh() == HIGH;
  sw3_status = FastGPIO::Pin<6>::isInputHigh() == HIGH;
  sw4_status = FastGPIO::Pin<7>::isInputHigh() == HIGH;
  return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);
}

void check_switch() {
  switch_status = read_switch_status();
  if (switch_status) {
    if (sw4_status) {
      displayAlarm(1);
    }

    if (sw1_status) {
      beep();
      delay(300);
      menu();
      delay(200);
    }

    else if (sw2_status) {
      displayDate();
    }

    else if (sw3_status) {
      displayTemperature();
    }
  }
}

void beep() {
  isAlarmActive = true;
  digitalWrite(buzzer_pin, HIGH);
  delay(beep_delay);
  digitalWrite(buzzer_pin, LOW);
}

void stopAlarm() {
  if (!isAlarmActive) {
    return;
  }


  // DateTime now = rtc.now();
  beep();

  rtc.disableAlarm(1);
}

void menu() {
  Serial.println("entered menu");
  delay(100);
  while (!read_switch_status()) {
    sendDataViaI2C("Enu==", "1");
    DateTime now = rtc.now();
    if (now.second() % 2) {
      sendDataViaI2C("-----", "0");
    } else {
      sendDataViaI2C("Enu==", "1");
    }
  }

  if (sw1_status) {

    delay(100);
    if (SetTime()) {
      return 1;
    } else if (SetDate()) {
      return 1;
    } else if (SetYear()) {
      return 1;
    } else if (SetAlarm()) {
      return 1;
    }
  } else {
    beep();
  }
}

int SetTime() {
  int time_chandged_status = 0;
  int my_hour_tmp = 0;
  DateTime now = rtc.now();
  int my_hour = now.hour();
  int my_min = now.minute();
  beep();
  delay(100);
  read_switch_status();

  delay(100);
  while (!sw1_status) {
    read_switch_status();
    if (sw2_status) {
      time_chandged_status = 1;
      delay(150);
      my_hour++;
      if (my_hour > 23) {
        my_hour = 0;
      }
    } else if (sw3_status) {
      time_chandged_status = 1;
      delay(100);
      my_min++;
      if (my_min > 59) {
        my_min = 0;
      }
    } else if (sw4_status) {
      beep();
      return 1;
    }
    const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";

    if (my_hour == 0) {
      my_hour_tmp = 12;
    } else if (my_hour > 12) {
      my_hour_tmp = my_hour - 12;
    }

    char dispString[6];
    snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], my_hour_tmp, my_min);
    sendDataViaI2C(dispString, "1");
  }
  delay(100);
  if (time_chandged_status) {
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

int SetDate() {
  int date_chandged_status = 0;
  DateTime now = rtc.now();
  int my_date = now.day();
  int my_month = now.month();
  int my_year = now.year();
  read_switch_status();
  beep();
  delay(100);
  while (!sw1_status) {
    read_switch_status();
    if (sw2_status) {
      date_chandged_status = 1;
      delay(150);
      my_date++;
      if (my_date > 31) {
        my_date = 0;
      }
    } else if (sw3_status) {
      date_chandged_status = 1;
      delay(150);
      my_month++;
      if (my_month > 12) {
        my_month = 0;
      }
    } else if (sw4_status) {
      beep();
      return 1;
    }

    char dateString[6];


    sprintf(dateString, "d%02d%02d", my_date, my_month);
    sendDataViaI2C(dateString, "1");

    delay(5000);


    char yearString[6];

    sprintf(yearString, "Y%04d", my_year);
    sendDataViaI2C(yearString, "0");
    delay(5000);
    return;
  }

  if (date_chandged_status) {
    DateTime now = rtc.now();
    int my_hour = now.hour();  // get the current time
    int my_min = now.minute();
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }
  getTime();
  delay(200);
  return 0;
}

int SetYear() {
  int year_chandged_status = 0;
  DateTime now = rtc.now();
  int my_year = now.year();
  read_switch_status();
  beep();
  while (!sw1_status) {
    read_switch_status();
    if (sw2_status) {
      year_chandged_status = 1;
      delay(150);
      my_year--;
      if (my_year < 2024) {
        my_year = 2024;
      }
    } else if (sw3_status) {
      year_chandged_status = 1;
      delay(150);
      my_year++;
      if (my_year > 2070) {
        my_year = 2070;
      }
    } else if (sw4_status) {
      beep();
      return 1;
    }

    char yearString[6];


    sprintf(yearString, "Y%04d", my_year);
    sendDataViaI2C(yearString, "0");
  }

  if (year_chandged_status) {
    DateTime now = rtc.now();
    int my_hour = now.hour();  // get the current time
    int my_min = now.minute();
    int my_date = now.day();  // get the current date
    int my_month = now.month();
    // now set the current time and date as it is and set updated year
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }
  getTime();
  delay(200);
  return 0;
}

int SetAlarm() {
  int alarm_chandged_status = 0;
  int alarm_hour = 0, alarm_min = 0, alarm_hour_temp = 0;
  read_switch_status();
  beep();
  while (!sw1_status) {
    read_switch_status();
    if (sw2_status) {
      alarm_chandged_status = 1;
      delay(150);
      alarm_hour++;
      if (alarm_hour > 23) {
        alarm_hour = 0;
      }
    } else if (sw3_status) {
      alarm_chandged_status = 1;
      delay(150);
      alarm_min++;
      if (alarm_min > 59) {
        alarm_min = 0;
      }
    } else if (sw4_status) {
      beep();
      return 1;
    }

    const char *apDigit = (alarm_hour > 12 && alarm_min > 0) ? "P" : "A";

    if (alarm_hour == 0) {
      alarm_hour_temp = 12;
    } else if (alarm_hour > 12) {
      alarm_hour_temp = alarm_hour - 12;
    }
    char dispString[6];
    snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], alarm_hour_temp, alarm_min);
    sendDataViaI2C(dispString, "1");
  }
  delay(100);
  rtc.disableAlarm(1);
  rtc.clearAlarm(1);
  if (alarm_chandged_status) {
    rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
    getTime();
    beep();
    delay(200);
    return 0;
  }
  return 0;
}

void clearAll() {
  sendDataViaI2C("=====", "0");
};

void setup() {
  FastGPIO::Pin<4>::setInput();
  FastGPIO::Pin<5>::setInput();
  FastGPIO::Pin<6>::setInput();
  FastGPIO::Pin<7>::setInput();
  FastGPIO::Pin<buzzer_pin>::setOutput(0);
  Wire.begin();
  Serial.begin(115200);
  Serial.println("Master ready");
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(alarmStopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(alarmStopPin), stopAlarm, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin), beep, FALLING);
  Serial.println(F("setup(): begin"));
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
  isAlarmActive = false;
  DateTime now = rtc.now();

  char buff[] = "Start time is hh:mm:ss DDD, DD MMM YYYY";
  Serial.println(now.toString(buff));


  clearAll();
  Serial.println(F("setup(): ready"));
}

void loop() {
  getTime();
  if (isAlarmActive) {
    beep();
  }
}
