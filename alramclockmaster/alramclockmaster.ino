#include <FastGPIO.h>
#include <Wire.h>
#include "RTClib.h"

// Class RTCDisplay handles the Real Time Clock (RTC) functionalities and display.
class RTCDisplay {
private:
  RTC_DS3231 rtc;                    // Instance of RTC_DS3231 to manage RTC operations.
  const byte displayAddress = 0x2A;  // I2C address of the display.
  const int buzzerPin = 13;          // Pin number for the buzzer.
  boolean isAlarmActive = false;     // Flag to check if the alarm is active.

public:
  // Constructor to initialize the RTC and set up communication.
  RTCDisplay() {
    Wire.begin();          // Start I2C communication.
    Serial.begin(115200);  // Start serial communication at 115200 baud rate.

    // Check if the RTC is connected.
    if (!rtc.begin()) {
      Serial.println("Couldn't find RTC");  // Error message if RTC not found.
      Serial.flush();                       // Ensure all serial data is sent.
      abort();                              // Stop the execution if RTC is not found.
    }

    // Disable and clear alarms and set SQW pin mode.
    rtc.disableAlarm(1);
    rtc.disableAlarm(2);
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);
    rtc.writeSqwPinMode(DS3231_OFF);               // Disable square wave output.
    Serial.println(F("RTC Display Initialized"));  // Initialization message.
  }

  // Sends data to the display via I2C.
  void sendDataViaI2C(const char *data) {
    // Check if the data length is exactly 6 characters.
    if (strlen(data) != 6) {
      Serial.println("Error: Data must be exactly 6 characters long.");  // Error message.
      return;                                                            // Exit function if data length is incorrect.
    }

    // Begin transmission to the display.
    // Wire.beginTransmission(displayAddress);
    // Wire.write(data);             // Send the data.
    // Wire.endTransmission();       // End transmission.
    Serial.print("Data sent: ");  // Log sent data.
    Serial.println(data);
  }

  // Display the current temperature.
  void displayTemperature() {
    char dispString[6];                                          // Buffer to hold the formatted string.
    int tempInt = static_cast<int>(rtc.getTemperature() * 100);  // Get temperature in centi-degrees.
    sprintf(dispString, "t%04d1", tempInt);                      // Format the temperature string.
    sendDataRepeatedly(dispString);                              // Send data repeatedly to the display.
  }

  // Display the alarm time.
  void displayAlarm(int alarm) {
    char alarmDateString[6];                                                                      // Buffer to hold the alarm time string.
    DateTime alarmTime = (alarm == 1) ? rtc.getAlarm1() : rtc.getAlarm2();                        // Get the specified alarm time.
    int my_hour = alarmTime.hour();                                                               // Extract hour from alarm time.
    int my_min = alarmTime.minute();                                                              // Extract minute from alarm time.
    char apDigit = (my_hour >= 12) ? 'P' : 'A';                                                   // Determine AM/PM indicator.
    my_hour = (my_hour > 12) ? my_hour - 12 : my_hour;                                            // Convert to 12-hour format.
    snprintf(alarmDateString, sizeof(alarmDateString), "%c%02d%02d1", apDigit, my_hour, my_min);  // Format alarm time string.
    sendDataRepeatedly(alarmDateString);                                                          // Send alarm time to the display.
  }

  // Display the current date.
  void displayDate() {
    DateTime now = rtc.now();                                   // Get the current date and time.
    char dateString[6];                                         // Buffer to hold the date string.
    sprintf(dateString, "d%02d%02d1", now.day(), now.month());  // Format the date string.
    sendDataRepeatedly(dateString);                             // Send date to the display.

    char yearString[6];                         // Buffer to hold the year string.
    sprintf(yearString, "Y%04d0", now.year());  // Format the year string.
    sendDataRepeatedly(yearString);             // Send year to the display.
  }

  DateTime getCurrentTime() {
    return rtc.now();  // Return the current time from the RTC.
  }

  void adjust(DateTime dt) {
    rtc.adjust(dt);  // Adjust the RTC with the provided DateTime.
  }

  void setAlarm1(DateTime dt) {
    rtc.setAlarm1(dt, DS3231_A1_Hour);
  }

  void disableAlarm(int alramIndex) {
    rtc.disableAlarm(alramIndex);
  }
  void clearAlarm(int alramIndex) {
    rtc.disableAlarm(alramIndex);
  }

  // Get the current time and send it to the display.
  void getTime() {
    DateTime now = rtc.now();                           // Get the current date and time.
    int my_hour = now.hour();                           // Extract hour.
    int my_min = now.minute();                          // Extract minute.
    const char *apDigit = (my_hour >= 12) ? "P" : "A";  // Determine AM/PM indicator.
    if (my_hour == 0) my_hour = 12;                     // Convert to 12-hour format.
    else if (my_hour > 12) my_hour -= 12;               // Convert to 12-hour format.

    char dispString[6];                                                                    // Buffer to hold the time string.
    snprintf(dispString, sizeof(dispString), "%c%02d%02d2", apDigit[0], my_hour, my_min);  // Format time string.
    sendDataViaI2C(dispString);                                                            // Send time to the display via I2C.
  }

  void alramActivated() {
    isAlarmActive = true;  // Set alarm active flag.
    beep(true);
  }

  // Activate the buzzer for an alarm.
  void beep(bool continuous = false, int beepDelay = 20) {
    if (continuous) {
      // Continuous beeping: beep twice a second.
      while (isAlarmActive) {           // Use a flag to check if the alarm should continue beeping.
        digitalWrite(buzzerPin, HIGH);  // Turn on the buzzer.
        delay(beepDelay);               // Wait for beep duration.
        digitalWrite(buzzerPin, LOW);   // Turn off the buzzer.
        delay(beepDelay);               // Wait for the interval before the next beep (to achieve twice a second).
      }
    } else {
      // Short beep: single beep.
      digitalWrite(buzzerPin, HIGH);  // Turn on the buzzer.
      delay(beepDelay);               // Wait for beep duration.
      digitalWrite(buzzerPin, LOW);   // Turn off the buzzer.
    }
  }

  // Stop the alarm if it is active.
  void stopAlarm() {
    if (!isAlarmActive) return;  // Exit if no alarm is active.
    beep();                      // Activate the beep.
    rtc.disableAlarm(1);         // Disable the first alarm.
  }

private:
  // Sends data repeatedly to the display for a specified duration.
  void sendDataRepeatedly(const char *data) {
    long startTime = millis();  // Record the start time.
    // Send data for 5 seconds.
    while ((millis() - startTime) <= 5000) {
      sendDataViaI2C(data);  // Send data via I2C.
    }
  }
};

// Class SwitchHandler handles the state of switches.
class SwitchHandler {
private:
  // Status of the switches.
  boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0, sw5_status = 0;

public:
  // Read the status of each switch.
  void readSwitchStatus() {
    sw5_status = FastGPIO::Pin<3>::isInputHigh() == HIGH;  // Read switch 5 status.
    sw1_status = FastGPIO::Pin<4>::isInputHigh() == HIGH;  // Read switch 1 status.
    sw2_status = FastGPIO::Pin<5>::isInputHigh() == HIGH;  // Read switch 2 status.
    sw3_status = FastGPIO::Pin<6>::isInputHigh() == HIGH;  // Read switch 3 status.
    sw4_status = FastGPIO::Pin<7>::isInputHigh() == HIGH;  // Read switch 4 status.
  }

  // Check if any switch is active.
  boolean anySwitchActive() {
    return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);  // Return true if any switch is active.
  }

  // Get the status of switch 1.
  boolean getSw1Status() {
    return sw1_status;  // Return status of switch 1.
  }

  // Get the status of switch 2.
  boolean getSw2Status() {
    return sw2_status;  // Return status of switch 2.
  }

  // Get the status of switch 3.
  boolean getSw3Status() {
    return sw3_status;  // Return status of switch 3.
  }

  // Get the status of switch 4.
  boolean getSw4Status() {
    return sw4_status;  // Return status of switch 4.
  }

  // Get the status of switch 5.
  boolean getSw5Status() {
    return sw5_status;  // Return status of switch 5.
  }
};

// Class Menu handles the menu operations for setting time, date, and alarms.
class Menu {
private:
  RTCDisplay &rtcDisplay;        // Reference to the RTCDisplay instance.
  SwitchHandler &switchHandler;  // Reference to the SwitchHandler instance.

public:
  // Constructor to initialize the menu with RTCDisplay and SwitchHandler references.
  Menu(RTCDisplay &rtc, SwitchHandler &sw)
    : rtcDisplay(rtc), switchHandler(sw) {}

  // Display the main menu.
  void displayMenu() {
    Serial.println("Entered menu");  // Log the entry into the menu.
    delay(100);                      // Short delay.
    // Loop until any switch is activated.
    while (!switchHandler.anySwitchActive()) {
      rtcDisplay.sendDataViaI2C("Enu==1");         // Send menu display command.
      DateTime now = rtcDisplay.getCurrentTime();  // Get the current time.
      // Blink the menu display every second.
      if (now.second() % 2) {
        rtcDisplay.sendDataViaI2C("-----1");  // Send dash display.
      } else {
        rtcDisplay.sendDataViaI2C("Enu==1");  // Send menu display command.
      }
    }

    // Check if switch 1 is active to enter the next menu.
    if (switchHandler.getSw1Status()) {
      rtcDisplay.beep();  // Beep to alert user.
      delay(300);         // Delay before entering sub-menu.
      menu();             // Call the menu function.
      delay(200);         // Delay after returning from sub-menu.
    } else {
      rtcDisplay.beep();  // Beep if no switch is activated.
    }
  }

  // Function to manage the menu options.
  void menu() {
    Serial.println("entered menu");  // Log entry into the menu.
    delay(100);                      // Short delay.
    // Loop until any switch is activated.
    while (!switchHandler.anySwitchActive()) {
      rtcDisplay.sendDataViaI2C("Enu==1");         // Send menu display command.
      DateTime now = rtcDisplay.getCurrentTime();  // Get the current time.
      // Blink the menu display every second.
      if (now.second() % 2) {
        rtcDisplay.sendDataViaI2C("-----1");  // Send dash display.
      } else {
        rtcDisplay.sendDataViaI2C("Enu==1");  // Send menu display command.
      }
    }

    // Check if switch 1 is active to enter options for time, date, or alarm setting.
    if (switchHandler.getSw1Status()) {
      delay(100);               // Short delay.
      if (SetTime()) {          // Set time if switch 2 is pressed.
        return;                 // Exit if time is set.
      } else if (SetDate()) {   // Set date if switch 3 is pressed.
        return;                 // Exit if date is set.
      } else if (SetYear()) {   // Set year if switch 4 is pressed.
        return;                 // Exit if year is set.
      } else if (SetAlarm()) {  // Set alarm if switch 5 is pressed.
        return;                 // Exit if alarm is set.
      }
    } else {
      rtcDisplay.beep();  // Beep if no switch is activated.
    }
  }

  // Set the current time.
  int SetTime() {
    int time_changed_status = 0;                 // Status to check if time is changed.
    int my_hour_tmp = 0;                         // Temporary variable for hour.
    DateTime now = rtcDisplay.getCurrentTime();  // Get current time.
    int my_hour = now.hour();                    // Get hour.
    int my_min = now.minute();                   // Get minute.

    rtcDisplay.beep();                 // Alert user with a beep.
    delay(100);                        // Short delay.
    switchHandler.readSwitchStatus();  // Read switch statuses.

    delay(100);  // Short delay.
    // Loop until switch 1 is pressed to exit time setting.
    while (!switchHandler.getSw1Status()) {
      switchHandler.readSwitchStatus();  // Update switch statuses.
      // Increment hour if switch 2 is pressed.
      if (switchHandler.getSw2Status()) {
        time_changed_status = 1;  // Mark time as changed.
        delay(150);               // Short delay.
        my_hour++;                // Increment hour.
        if (my_hour > 23) {       // Wrap around if hour exceeds 23.
          my_hour = 0;            // Reset hour to 0.
        }
      }
      // Increment minute if switch 3 is pressed.
      else if (switchHandler.getSw3Status()) {
        time_changed_status = 1;  // Mark time as changed.
        delay(100);               // Short delay.
        my_min++;                 // Increment minute.
        if (my_min > 59) {        // Wrap around if minute exceeds 59.
          my_min = 0;             // Reset minute to 0.
        }
      }
      // Exit if switch 4 is pressed.
      else if (switchHandler.getSw4Status()) {
        rtcDisplay.beep();  // Alert user with a beep.
        return 1;           // Exit the function.
      }

      // Determine AM/PM indicator for display.
      const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";

      // Convert to 12-hour format for display.
      if (my_hour == 0) {
        my_hour_tmp = 12;  // Midnight case.
      } else if (my_hour > 12) {
        my_hour_tmp = my_hour - 12;  // Convert to 12-hour format.
      }

      char dispString[6];                                                                        // Buffer to hold the time string.
      snprintf(dispString, sizeof(dispString), "%c%02d%02d1", apDigit[0], my_hour_tmp, my_min);  // Format time string.
      rtcDisplay.sendDataViaI2C(dispString);                                                     // Send formatted time to display.
    }

    delay(100);  // Short delay.
    // If time was changed, adjust the RTC.
    if (time_changed_status) {
      DateTime now = rtcDisplay.getCurrentTime();                                   // Get current date.
      int my_date = now.day();                                                      // Get current day.
      int my_month = now.month();                                                   // Get current month.
      int my_year = now.year();                                                     // Get current year.
      rtcDisplay.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));  // Adjust RTC with new time.
      rtcDisplay.beep();                                                            // Alert user with a beep.
      delay(50);                                                                    // Short delay.
    }

    rtcDisplay.getTime();  // Refresh the display with current time.
    delay(200);            // Short delay.
    return 0;              // Return to caller.
  }

  // Set the current date.
  int SetDate() {
    int date_changed_status = 0;                 // Status to check if date is changed.
    DateTime now = rtcDisplay.getCurrentTime();  // Get current date.
    int my_date = now.day();                     // Get current day.
    int my_month = now.month();                  // Get current month.
    int my_year = now.year();                    // Get current year.
    switchHandler.readSwitchStatus();            // Read switch statuses.
    rtcDisplay.beep();                           // Alert user with a beep.
    delay(100);                                  // Short delay.
    // Loop until switch 1 is pressed to exit date setting.
    while (!switchHandler.getSw1Status()) {
      switchHandler.readSwitchStatus();  // Update switch statuses.
      // Increment day if switch 2 is pressed.
      if (switchHandler.getSw2Status()) {
        date_changed_status = 1;  // Mark date as changed.
        delay(150);               // Short delay.
        my_date++;                // Increment day.
        if (my_date > 31) {       // Wrap around if day exceeds 31.
          my_date = 1;            // Reset day to 1.
        }
      }
      // Increment month if switch 3 is pressed.
      else if (switchHandler.getSw3Status()) {
        date_changed_status = 1;  // Mark date as changed.
        delay(150);               // Short delay.
        my_month++;               // Increment month.
        if (my_month > 12) {      // Wrap around if month exceeds 12.
          my_month = 1;           // Reset month to 1.
        }
      }
      // Exit if switch 4 is pressed.
      else if (switchHandler.getSw4Status()) {
        rtcDisplay.beep();  // Alert user with a beep.
        return 1;           // Exit the function.
      }

      char dateString[6];                                    // Buffer to hold the date string.
      sprintf(dateString, "d%02d%02d1", my_date, my_month);  // Format date string.
      rtcDisplay.sendDataViaI2C(dateString);                 // Send formatted date to display.

      delay(5000);  // Delay before refreshing display.

      char yearString[6];                      // Buffer to hold the year string.
      sprintf(yearString, "Y%04d0", my_year);  // Format year string.
      rtcDisplay.sendDataViaI2C(yearString);   // Send formatted year to display.
      delay(5000);                             // Delay before refreshing display.
    }

    // If date was changed, adjust the RTC.
    if (date_changed_status) {
      DateTime now = rtcDisplay.getCurrentTime();                                   // Get current time.
      int my_hour = now.hour();                                                     // Get current hour.
      int my_min = now.minute();                                                    // Get current minute.
      rtcDisplay.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));  // Adjust RTC with new date.
      rtcDisplay.beep();                                                            // Alert user with a beep.
      delay(50);                                                                    // Short delay.
    }

    rtcDisplay.getTime();  // Refresh the display with current time.
    delay(200);            // Short delay.
    return 0;              // Return to caller.
  }

  // Set the current year.
  int SetYear() {
    int year_changed_status = 0;                 // Status to check if year is changed.
    DateTime now = rtcDisplay.getCurrentTime();  // Get current date.
    int my_year = now.year();                    // Get current year.
    switchHandler.readSwitchStatus();            // Read switch statuses.
    rtcDisplay.beep();                           // Alert user with a beep.
    // Loop until switch 1 is pressed to exit year setting.
    while (!switchHandler.getSw1Status()) {
      switchHandler.readSwitchStatus();  // Update switch statuses.
      // Decrement year if switch 2 is pressed.
      if (switchHandler.getSw2Status()) {
        year_changed_status = 1;  // Mark year as changed.
        delay(150);               // Short delay.
        my_year--;                // Decrement year.
        if (my_year < 2024) {     // Limit year to a minimum of 2024.
          my_year = 2024;         // Reset year to 2024.
        }
      }
      // Increment year if switch 3 is pressed.
      else if (switchHandler.getSw3Status()) {
        year_changed_status = 1;  // Mark year as changed.
        delay(150);               // Short delay.
        my_year++;                // Increment year.
        if (my_year > 2070) {     // Limit year to a maximum of 2070.
          my_year = 2070;         // Reset year to 2070.
        }
      }
      // Exit if switch 4 is pressed.
      else if (switchHandler.getSw4Status()) {
        rtcDisplay.beep();  // Alert user with a beep.
        return 1;           // Exit the function.
      }

      char yearString[6];                      // Buffer to hold the year string.
      sprintf(yearString, "Y%04d0", my_year);  // Format year string.
      rtcDisplay.sendDataViaI2C(yearString);   // Send formatted year to display.
    }

    // If year was changed, adjust the RTC.
    if (year_changed_status) {
      DateTime now = rtcDisplay.getCurrentTime();                                   // Get current time.
      int my_hour = now.hour();                                                     // Get current hour.
      int my_min = now.minute();                                                    // Get current minute.
      int my_date = now.day();                                                      // Get current day.
      int my_month = now.month();                                                   // Get current month.
      rtcDisplay.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));  // Adjust RTC with new year.
      rtcDisplay.beep();                                                            // Alert user with a beep.
      delay(50);                                                                    // Short delay.
    }

    rtcDisplay.getTime();  // Refresh the display with current time.
    delay(200);            // Short delay.
    return 0;              // Return to caller.
  }

  // Set the alarm time.
  int SetAlarm() {
    int alarm_changed_status = 0;                            // Status to check if alarm time is changed.
    int alarm_hour = 0, alarm_min = 0, alarm_hour_temp = 0;  // Variables to hold alarm hour and minute.
    switchHandler.readSwitchStatus();                        // Read switch statuses.
    rtcDisplay.beep();                                       // Alert user with a beep.
    // Loop until switch 1 is pressed to exit alarm setting.
    while (!switchHandler.getSw1Status()) {
      switchHandler.readSwitchStatus();  // Update switch statuses.
      // Increment alarm hour if switch 2 is pressed.
      if (switchHandler.getSw2Status()) {
        alarm_changed_status = 1;  // Mark alarm time as changed.
        delay(150);                // Short delay.
        alarm_hour++;              // Increment alarm hour.
        if (alarm_hour > 23) {     // Wrap around if hour exceeds 23.
          alarm_hour = 0;          // Reset hour to 0.
        }
      }
      // Increment alarm minute if switch 3 is pressed.
      else if (switchHandler.getSw3Status()) {
        alarm_changed_status = 1;  // Mark alarm time as changed.
        delay(150);                // Short delay.
        alarm_min++;               // Increment alarm minute.
        if (alarm_min > 59) {      // Wrap around if minute exceeds 59.
          alarm_min = 0;           // Reset minute to 0.
        }
      }
      // Exit if switch 4 is pressed.
      else if (switchHandler.getSw4Status()) {
        rtcDisplay.beep();  // Alert user with a beep.
        return 1;           // Exit the function.
      }

      // Determine AM/PM indicator for display.
      const char *apDigit = (alarm_hour > 12 && alarm_min > 0) ? "P" : "A";

      // Convert to 12-hour format for display.
      if (alarm_hour == 0) {
        alarm_hour_temp = 12;  // Midnight case.
      } else if (alarm_hour > 12) {
        alarm_hour_temp = alarm_hour - 12;  // Convert to 12-hour format.
      }
      char dispString[6];                                                                              // Buffer to hold the alarm time string.
      snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], alarm_hour_temp, alarm_min);  // Format alarm time string.
      rtcDisplay.sendDataViaI2C(dispString);                                                           // Send formatted alarm time to display.
    }

    delay(100);                  // Short delay.
    rtcDisplay.disableAlarm(1);  // Disable the first alarm.
    rtcDisplay.clearAlarm(1);    // Clear the first alarm.
    // If alarm time was changed, set the alarm in the RTC.
    if (alarm_changed_status) {
      rtcDisplay.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0));  // Set the alarm for specified hour and minute.
      rtcDisplay.getTime();                                               // Refresh the display with current time.
      rtcDisplay.beep();                                                  // Alert user with a beep.
      delay(200);                                                         // Short delay.
      return 0;                                                           // Return to caller.
    }
    return 0;  // Return to caller.
  }

  // Clear all settings and reset display.
  void clearAll() {
    rtcDisplay.sendDataViaI2C("=====0");  // Send command to clear all settings.
  }
};


// Create instances of the classes
RTCDisplay RTCDisplay;        // Automatic variable
SwitchHandler SwitchHandler;  // Automatic variable
Menu Menu(RTCDisplay, SwitchHandler);


void stopAlarmHandler() {
  RTCDisplay.stopAlarm();
}

void beepHandler() {
  RTCDisplay.beep();
}

// Arduino setup function which runs once at the start.
void setup() {
  // Set input and output pins for switches and buzzer.
  FastGPIO::Pin<4>::setInput();     // Set switch 1 as input.
  FastGPIO::Pin<5>::setInput();     // Set switch 2 as input.
  FastGPIO::Pin<6>::setInput();     // Set switch 3 as input.
  FastGPIO::Pin<7>::setInput();     // Set switch 4 as input.
  FastGPIO::Pin<13>::setOutput(0);  // Set buzzer pin as output.

  // Setup interrupts for button presses.
  pinMode(2, INPUT_PULLUP);  // Set pin 2 as input for interrupt.
  pinMode(3, INPUT_PULLUP);  // Set pin 3 as input for interrupt.

  // Attach interrupt for switch 3 to stop alarm on falling edge.
  attachInterrupt(digitalPinToInterrupt(3), stopAlarmHandler, FALLING);

  // Attach interrupt for switch 2 to beep on falling edge.
  attachInterrupt(digitalPinToInterrupt(2), beepHandler, FALLING);
  Wire.begin();          // Start I2C communication.
  Serial.begin(115200);  // Start serial communication at 115200 baud rate.

  // Check if the RTC is connected.
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");  // Error message if RTC not found.
    Serial.flush();                       // Ensure all serial data is sent.
    abort();                              // Stop the execution if RTC is not found.
  }

  // Disable and clear alarms and set SQW pin mode.
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF);               // Disable square wave output.
  Serial.println(F("RTC Display Initialized"));  // Initialization message.
}

// Arduino loop function which runs repeatedly after setup.
void loop() {
  Serial.print("Data sent: ");
  SwitchHandler.readSwitchStatus();  // Read the status of all switches.
  RTCDisplay.getTime();              // Get and display the current time.
  // If any switch is active, display the menu.
  if (SwitchHandler.anySwitchActive()) {
    Menu.displayMenu();  // Call displayMenu method of Menu instance.
  }
}