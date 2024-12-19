#include <FastGPIO.h>
#include <Wire.h>
#include "RTClib.h"

// RTC instance
RTC_DS3231 rtc;

// I2C address for the display
#define DISPLAY_ADDRESS 0x2A

// Pin definitions
const byte interruptPin = 2;
const byte alarmStopPin = 3;
const byte buzzerPin = 13;

// Global variables
volatile byte state = LOW;
int beep_delay = 20;
boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0, sw5_status = 0;
boolean isAlarmActive = false;

class DisplayManager {
public:
    void sendDataViaI2C(const char *data, char dotStatus = '0') {
        // Validate dotStatus input
        if (dotStatus != '0' && dotStatus != '1' && dotStatus != '2') {
            Serial.println("Error: Invalid dotStatus value. Must be '0', '1', or '2'.");
            return;
        }

        // Validate data length
        if (strlen(data) != 5) {
            Serial.println("Error: Data must be exactly 5 characters long.");
            return;
        }

        // Create a buffer to hold the data and the dot status
        char buffer[7]; // 5 characters + 1 dot status + 1 null terminator

        // Copy the original data to the buffer
        strcpy(buffer, data);

        // Append the dot status character
        buffer[5] = dotStatus;
        buffer[6] = '\0'; // Null-terminate the string

        // Start communication with the slave
        Wire.beginTransmission(DISPLAY_ADDRESS);
        // Send the modified string
        Wire.write(buffer);
        // End communication
        Wire.endTransmission();

        // Print the data being sent for debugging
        Serial.print("Data sent: ");
        Serial.println(buffer);
    }

    void displayTemperature() {
        char dispString[6]; // Enough space for "txxxx\0"

        // Convert the temperature to an integer representation
        int tempInt = static_cast<int>(rtc.getTemperature() * 100); // Convert to integer (e.g., 26.54 -> 2654)

        // Format the string as "txxxx" where xxxx is the temperature
        sprintf(dispString, "t%04d", tempInt);

        long t = millis();
        while ((millis() - t) <= 5000) { // Display for 5 seconds
            sendDataViaI2C(dispString, '1'); // Call the function to display the formatted string
        }
    }

    void displayAlarm(int alarm) {
        char alarmDateString[6]; // Enough space for "A1234\0" or "P1234\0"
        DateTime alarmTime = (alarm == 1) ? rtc.getAlarm1() : rtc.getAlarm2(); // Get the stored alarm value
        int my_hour = alarmTime.hour(); // Get the hour
        int my_min = alarmTime.minute(); // Get the minute

        // Determine AM/PM and adjust hour
        char apDigit = (my_hour >= 12) ? 'P' : 'A'; // 'P' for PM, 'A' for AM
        my_hour = (my_hour > 12) ? my_hour - 12 : my_hour; // Convert to 12-hour format

        // Format the alarm time as "A1234" or "P1234"
        snprintf(alarmDateString, sizeof(alarmDateString), "%c%02d%02d", apDigit, my_hour, my_min);

        // Display the alarm time for 5 seconds
        long startTime = millis();
        while ((millis() - startTime) <= 5000) {
            sendDataViaI2C(alarmDateString, '1');
        }
    }

    void displayDate() {
        DateTime now = rtc.now();
        int my_date = now.day();
        int my_month = now.month();
        int my_year = now.year(); // Get the full 4-digit year

        // Create a character array to hold the formatted date string
        char dateString[6]; // Enough space for "dDDMM\0"

        // Format the date as "dDDMM"
        sprintf(dateString, "d%02d%02d", my_date, my_month);

        // Display the date for 5 seconds
        long t = millis();
        while ((millis() - t) <= 5000) {
            sendDataViaI2C(dateString, '1');
        }

        // Create a character array for the year with prefix 'Y'
        char yearString[6]; // Enough space for "Yyyyy\0"

        // Format the year as "Yyyyy"
        sprintf(yearString, "Y%04d", my_year);

        // Display the year for 5 seconds
        t = millis();
        while ((millis() - t) <= 5000) {
            sendDataViaI2C(yearString, '0');
        }
    }

    void getTime() {
        DateTime now = rtc.now();
        int my_hour = now.hour();
        int my_min = now.minute();
        const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";

        // Convert 24-hour format to 12-hour format
        if (my_hour == 0) {
            my_hour = 12; // Midnight is 12 AM
        } else if (my_hour > 12) {
            my_hour -= 12; // Convert PM times
        }

        // Use fixed-size character array instead of String
        char dispString[6]; // "A1234\0" or "P1234\0"
        snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], my_hour, my_min);
        sendDataViaI2C(dispString, '2');
    }
};

class AlarmManager {
public:
    void stopAlarm() {
        if (!isAlarmActive) {
            return;
        }
        // The alarm has just fired

        DateTime now = rtc.now();
        // beep();
        //  Disable and clear alarm
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);

        // Perhaps reset to new time if required
        rtc.setAlarm1(now + TimeSpan(0, 0, 0, 10), DS3231_A1_Second); // Set for another 10 seconds
    }

    void beep() {
        isAlarmActive = true;
        digitalWrite(buzzerPin, HIGH);
        delay(beep_delay);
        digitalWrite(buzzerPin, LOW);
    }

    void menu() {
        Serial.println("entered menu");
        delay(100);
        while (!readSwitchStatus()) {
            displayManager.sendDataViaI2C("Enu==", '1');
            DateTime now = rtc.now();
            if (now.second() % 2) {
                displayManager.sendDataViaI2C("-----", '0'); // display an animation when entered into menu
            } else {
                displayManager.sendDataViaI2C("Enu==", '1');
            }
        }

        if (sw1_status) {
            // Set the clock parameters
            delay(100);
            if (setTime()) {
                return;
            } else if (setDate()) {
                return;
            } else if (setYear()) {
                return;
            } else if (setAlarm()) {
                return;
            }
        } else {
            beep();
        }
    }

    int setTime() {
        int timeChangedStatus = 0;
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
                timeChangedStatus = 1;
                delay(150);
                my_hour++;
                if (my_hour > 23) {
                    my_hour = 0;
                }
            } else if (sw3_status) {
                timeChangedStatus = 1;
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
            // Convert 24-hour format to 12-hour format
            if (my_hour == 0) {
                my_hour_tmp = 12; // Midnight is 12 AM
            } else if (my_hour > 12) {
                my_hour_tmp = my_hour - 12; // Convert PM times
            }
            // Use fixed-size character array instead of String
            char dispString[6]; // "A1234\0" or "P1234\0"
            snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], my_hour_tmp, my_min);
            displayManager.sendDataViaI2C(dispString, '1');
        }
        delay(100);
        if (timeChangedStatus) {
            DateTime now = rtc.now();
            int my_date = now.day(); // get the current date
            int my_month = now.month();
            // now set the current date as it is and set updated time
            rtc.adjust(DateTime(now.year(), my_month, my_date, my_hour, my_min, 0));
            beep();
            delay(50);
        }
        displayManager.getTime();
        delay(200);
        return 0;
    }

    int setDate() {
        int dateChangedStatus = 0;
        DateTime now = rtc.now();
        int my_date = now.day();
        int my_month = now.month();
        int my_year = now.year(); // Get the full 4-digit year
        readSwitchStatus();
        beep();
        delay(100);
        while (!sw1_status) {
            readSwitchStatus();
            if (sw2_status) {
                dateChangedStatus = 1;
                delay(150);
                my_date++;
                if (my_date > 31) {
                    my_date = 0;
                }
            } else if (sw3_status) {
                dateChangedStatus = 1;
                delay(150);
                my_month++;
                if (my_month > 12) {
                    my_month = 0;
                }
            } else if (sw4_status) {
                beep();
                return 1;
            }
            // Create a character array to hold the formatted date string
            char dateString[6]; // Enough space for "dDDMM\0"

            // Format the date as "dDDMM"
            sprintf(dateString, "d%02d%02d", my_date, my_month);
            displayManager.sendDataViaI2C(dateString, '1');
            // Display the date for 5 seconds
            delay(5000);

            // Create a character array for the year with prefix 'Y'
            char yearString[6]; // Enough space for "Yyyyy\0"

            // Format the year as "Yyyyy"
            sprintf(yearString, "Y%04d", my_year);
            displayManager.sendDataViaI2C(yearString, '0');
            // Display the year for 5 seconds
            delay(5000);
        }

        if (dateChangedStatus) {
            DateTime now = rtc.now();
            int my_hour = now.hour(); // get the current time
            int my_min = now.minute();
            rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
            beep();
            delay(50);
        }
        displayManager.getTime();
        delay(200);
        return 0;
    }

    int setYear() {
        int yearChangedStatus = 0;
        DateTime now = rtc.now();
        int my_year = now.year(); // Get the full 4-digit year
        readSwitchStatus();
        beep();
        while (!sw1_status) {
            readSwitchStatus();
            if (sw2_status) {
                yearChangedStatus = 1;
                delay(150);
                my_year--;
                if (my_year < 2024) {
                    my_year = 2024; // Constrain the year lower limit to 2024
                }
            } else if (sw3_status) {
                yearChangedStatus = 1;
                delay(150);
                my_year++;
                if (my_year > 2070) { // Constrain the year upper limit to 2070
                    my_year = 2070;
                }
            } else if (sw4_status) {
                beep();
                return 1;
            }
            // Create a character array for the year with prefix 'Y'
            char yearString[6]; // Enough space for "Yyyyy\0"

            // Format the year as "Yyyyy"
            sprintf(yearString, "Y%04d", my_year);
            displayManager.sendDataViaI2C(yearString, '0'); // Send the formatted year string to display
        }

        if (yearChangedStatus) {
            DateTime now = rtc.now();
            int my_hour = now.hour(); // get the current time
            int my_min = now.minute();
            int my_date = now.day(); // get the current date
            int my_month = now.month();
            // now set the current time and date as it is and set updated year
            rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
            beep();
            delay(50);
        }
        displayManager.getTime();
        delay(200);
        return 0;
    }

    int setAlarm() {
        int alarmChangedStatus = 0;
        int alarm_hour = 0, alarm_min = 0, alarm_hour_temp = 0;
        readSwitchStatus();
        beep();
        while (!sw1_status) {
            readSwitchStatus();
            if (sw2_status) {
                alarmChangedStatus = 1;
                delay(150);
                alarm_hour++;
                if (alarm_hour > 23) {
                    alarm_hour = 0;
                }
            } else if (sw3_status) {
                alarmChangedStatus = 1;
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
            // Convert 24-hour format to 12-hour format
            if (alarm_hour == 0) {
                alarm_hour_temp = 12; // Midnight is 12 AM
            } else if (alarm_hour > 12) {
                alarm_hour_temp = alarm_hour - 12; // Convert PM times
            }
            // Use fixed-size character array instead of String
            char dispString[6]; // "A1234\0" or "P1234\0"
            snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], alarm_hour_temp, alarm_min);
            displayManager.sendDataViaI2C(dispString, '1');
        }
        delay(100);
        rtc.disableAlarm(1);
        rtc.clearAlarm(1);

        rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
        displayManager.getTime();
        beep();
        delay(200);
        return 0;
    }

private:
    bool readSwitchStatus() {
        sw5_status = FastGPIO::Pin<3>::isInputHigh() == HIGH;
        sw1_status = FastGPIO::Pin<4>::isInputHigh() == HIGH;
        sw2_status = FastGPIO::Pin<5>::isInputHigh() == HIGH;
        sw3_status = FastGPIO::Pin<6>::isInputHigh() == HIGH;
        sw4_status = FastGPIO::Pin<7>::isInputHigh() == HIGH;
        return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);
    }
};

DisplayManager displayManager;
AlarmManager alarmManager;

void setup() {
    Serial.begin(115200);
    FastGPIO::Pin<4>::setInput();
    FastGPIO::Pin<5>::setInput();
    FastGPIO::Pin<6>::setInput();
    FastGPIO::Pin<7>::setInput();
    FastGPIO::Pin<13>::setOutput(0);
    Wire.begin();
    pinMode(interruptPin, INPUT_PULLUP);
    pinMode(alarmStopPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(alarmStopPin), []() { alarmManager.stopAlarm(); }, FALLING);
    attachInterrupt(digitalPinToInterrupt(interruptPin), []() { alarmManager.beep(); }, FALLING);

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
    Serial.println(F("setup(): ready"));
}

void loop() {
    displayManager.getTime();
    if (isAlarmActive) {
        alarmManager.beep();
    }
}
