#include <FastGPIO.h>
#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;
// Macro for I2C address
#define DISPLAY_ADDRESS 0x2A
const byte interruptPin = 2;
const byte alarmStopPin = 3;
volatile byte state = LOW;
int beep_delay = 20;
boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0, sw5_status = 0;
boolean isAlarmActive = false;
/**
 * Sends data via I2C with an appended dot status character.
 *
 * @param data The string data to send, expected to be exactly 5 characters.
 * @param dotStatus A character ('0', '1', or '2') representing the dot status.
 */
void sendDataViaI2C(const char *data, char dotStatus = 0)
{
  // Validate dotStatus input
  if (dotStatus != '0' && dotStatus != '1' && dotStatus != '2')
  {
    Serial.println("Error: Invalid dotStatus value. Must be '0', '1', or '2'.");
    return;
  }

  // Validate data length
  if (strlen(data) != 5)
  {
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

void displayTemperature()
{
  char dispString[6]; // Enough space for "txxxx\0"

  // Convert the temperature to an integer representation
  int tempInt = static_cast<int>(rtc.getTemperature() * 100); // Convert to integer (e.g., 26.54 -> 2654)

  // Format the string as "txxxx" where xxxx is the temperature
  sprintf(dispString, "t%04d", tempInt);

  long t = millis();
  while ((millis() - t) <= 5000)
  { // Display for 5 seconds
    // Display the string
    sendDataViaI2C(dispString, '1'); // Call the function to display the formatted string
  }
  return;
}

void displayAlarm(int alarm)
{
  char alarmDateString[6];                                               // Enough space for "A1234\0" or "P1234\0"
  DateTime alarmTime = (alarm == 1) ? rtc.getAlarm1() : rtc.getAlarm2(); // Get the stored alarm value
  int my_hour = alarmTime.hour();                                        // Get the hour
  int my_min = alarmTime.minute();                                       // Get the minute

  // Determine AM/PM and adjust hour
  char apDigit = (my_hour >= 12) ? 'P' : 'A';        // 'P' for PM, 'A' for AM
  my_hour = (my_hour > 12) ? my_hour - 12 : my_hour; // Convert to 12-hour format

  // Format the alarm time as "A1234" or "P1234"
  snprintf(alarmDateString, sizeof(alarmDateString), "%c%02d%02d", apDigit, my_hour, my_min);

  // Display the alarm time for 1.5 seconds
  long startTime = millis(); // Declare and initialize the timer
  while ((millis() - startTime) <= 5000)
  {
    sendDataViaI2C(alarmDateString, '1'); // Send the formatted string to the display
  }
  return;
}

void displayDate()
{
  long t = 0;
  DateTime now = rtc.now();
  int my_date = now.day();
  int my_month = now.month();
  int my_year = now.year(); // Get the full 4-digit year

  // Create a character array to hold the formatted date string
  char dateString[6]; // Enough space for "dDDMM\0"

  // Format the date as "dDDMM"
  sprintf(dateString, "d%02d%02d", my_date, my_month);

  // Display the date for 5 seconds
  t = millis();
  while ((millis() - t) <= 5000)
  {
    loopDisplay(dateString, true); // Send the formatted date string to display
  }

  // Create a character array for the year with prefix 'E'
  char yearString[6]; // Enough space for "Eyyyy\0"

  // Format the year as "Yyyyy"
  sprintf(yearString, "Y%04d", my_year);

  // Display the year for 0.5 seconds
  t = millis();
  while ((millis() - t) <= 5000)
  {
    sendDataViaI2C(yearString, '0'); // Send the formatted year string to display
  }
  return;
}

void getTime()
{
  DateTime now = rtc.now();
  int my_hour = now.hour();
  int my_min = now.minute();
  const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";
  // Convert 24-hour format to 12-hour format
  if (my_hour == 0)
  {
    my_hour = 12; // Midnight is 12 AM
  }
  else if (my_hour > 12)
  {
    my_hour -= 12; // Convert PM times
  }
  // Use fixed-size character array instead of String
  char dispString[6]; // "A1234\0" or "P1234\0"
  snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], my_hour, my_min);
  sendDataViaI2C(dispString, '2');
}

boolean read_switch_status()
{
  sw5_status = FastGPIO::Pin<3>::isInputHigh() == HIGH;
  sw1_status = FastGPIO::Pin<4>::isInputHigh() == HIGH;
  sw2_status = FastGPIO::Pin<5>::isInputHigh() == HIGH;
  sw3_status = FastGPIO::Pin<6>::isInputHigh() == HIGH;
  sw4_status = FastGPIO::Pin<7>::isInputHigh() == HIGH;
  return (sw1_status || sw2_status || sw3_status || sw4_status || sw5_status);
}

void check_switch()
{
  switch_status = read_switch_status();
  if (switch_status)
  {
    if (sw4_status)
    {
      display_alarm();
    }

    if (sw1_status)
    {
      beep();
      delay(300);
      menu(); // go to menu if switch one is pressed
      delay(200);
    }

    else if (sw2_status)
    {
      display_date(); // display date if switch 2 is pressed
    }

    else if (sw3_status)
    {
      display_temperature(); // display tempreature if switch 3 is pressed
    }
  }
}

void beep()
{
  isAlarmActive = true;
  digitalWrite(buzzer_pin, HIGH);
  delay(beep_delay);
  digitalWrite(buzzer_pin, LOW);
}

void stopAlarm()
{
  if (!isAlarmActive)
  {
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

void menu()
{
  Serial.println("entered menu");
  delay(100);
  while (!read_switch_status())
  {
    sendDataViaI2C('Enu==', '1');
    DateTime now = rtc.now();
    if (now.second() % 2)
    {
      sendDataViaI2C('-----', '0'); // display an animation when entered into menu
    }
    else
    {
      sendDataViaI2C('Enu==', '1');
    }
  }

  if (sw1_status)
  {
    // Set the clock parameters
    delay(100);
    if (set_time())
    {
      return 1;
    }
    else if (set_date())
    {
      return 1;
    }
    else if (set_year())
    {
      return 1;
    }
    else if (set_alarm())
    {
      return 1;
    }
  }
  else
  {
    beep();
  }
}

int set_time()
{
  int time_chandged_status = 0;
  int my_hour_tmp = 0;
  DateTime now = rtc.now();
  int my_hour = now.hour();
  int my_min = now.minute();
  beep();
  delay(100);
  read_switch_status();

  delay(100);
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      time_chandged_status = 1;
      delay(150);
      my_hour++;
      if (my_hour > 23)
      {
        my_hour = 0;
      }
    }
    else if (sw3_status)
    {
      time_chandged_status = 1;
      delay(100);
      my_min++;
      if (my_min > 59)
      {
        my_min = 0;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";
    // Convert 24-hour format to 12-hour format
    if (my_hour == 0)
    {
      my_hour_tmp = 12; // Midnight is 12 AM
    }
    else if (my_hour > 12)
    {
      my_hour_tmp = my_hour - 12; // Convert PM times
    }
    // Use fixed-size character array instead of String
    char dispString[6]; // "A1234\0" or "P1234\0"
    snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], my_hour_tmp, my_min);
    sendDataViaI2C(dispString, '1');
  }
  delay(100);
  if (time_chandged_status)
  {
    DateTime now = rtc.now();
    my_date = now.day(); // get the current date
    my_month = now.month();
    // now set the current date as it is and set updated time
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }
  getTime();
  delay(200);
  return 0;
}

int set_date()
{
  int date_chandged_status = 0;
  DateTime now = rtc.now();
  int my_date = now.day();
  int my_month = now.month();
  int my_year = now.year(); // Get the full 4-digit year
  read_switch_status();
  beep();
  delay(100);
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      date_chandged_status = 1;
      delay(150);
      my_date++;
      if (my_date > 31)
      {
        my_date = 0;
      }
    }
    else if (sw3_status)
    {
      date_chandged_status = 1;
      delay(150);
      my_month++;
      if (my_month > 12)
      {
        my_month = 0;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    // Create a character array to hold the formatted date string
    char dateString[6]; // Enough space for "dDDMM\0"

    // Format the date as "dDDMM"
    sprintf(dateString, "d%02d%02d", my_date, my_month);
    sendDataViaI2C(dateString, '1');
    // Display the date for 5 seconds
    delay(5000);

    // Create a character array for the year with prefix 'E'
    char yearString[6]; // Enough space for "Eyyyy\0"

    // Format the year as "Yyyyy"
    sprintf(yearString, "Y%04d", my_year);
    sendDataViaI2C(yearString, '0');
    // Display the year for 0.5 seconds
    delay(5000);
    return;
  }

  if (date_chandged_status)
  {
    DateTime now = rtc.now();
    my_hour = now.hour(); // get the current time
    my_min = now.minute();
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }
  getTime();
  delay(200);
  return 0;
}

int set_year()
{
  int year_chandged_status = 0;
  DateTime now = rtc.now();
  int my_year = now.year(); // Get the full 4-digit year
  read_switch_status();
  beep();
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      year_chandged_status = 1;
      delay(150);
      my_year--;
      if (my_year < 2024)
      {
        my_year = 2024; // Constrain the year lower limit to 2000
      }
    }
    else if (sw3_status)
    {
      year_chandged_status = 1;
      delay(150);
      my_year++;
      if (my_year > 2070) // Constrain the year upper limit to 2070
      {
        my_year = 2070;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    // Create a character array for the year with prefix 'E'
    char yearString[6]; // Enough space for "Eyyyy\0"

    // Format the year as "Yyyyy"
    sprintf(yearString, "Y%04d", my_year);
    sendDataViaI2C(yearString, '0'); // Send the formatted year string to display
  }

  if (year_chandged_status)
  {
    DateTime now = rtc.now();
    my_hour = now.hour(); // get the current time
    my_min = now.minute();
    my_date = now.day(); // get the current date
    my_month = now.month();
    // now set the current time and date as it is and set updated year
    rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
    beep();
    delay(50);
  }
  getTime();
  delay(200);
  return 0;
}

int set_alarm()
{
  int alarm_chandged_status = 0;
  int alarm_hour = 0, alarm_min = 0, alarm_hour_temp = 0;
  read_switch_status();
  beep();
  while (!sw1_status)
  {
    read_switch_status();
    if (sw2_status)
    {
      alarm_chandged_status = 1;
      delay(150);
      alarm_hour++;
      if (alarm_hour > 23)
      {
        alarm_hour = 0;
      }
    }
    else if (sw3_status)
    {
      alarm_chandged_status = 1;
      delay(150);
      alarm_min++;
      if (alarm_min > 59)
      {
        alarm_min = 0;
      }
    }
    else if (sw4_status)
    {
      beep();
      return 1;
    }
    // int_to_string(alarm_hour, alarm_min);
    // send_to_display();
    // digitalWrite(dot, HIGH);
    const char *apDigit = (alarm_hour > 12 && alarm_min > 0) ? "P" : "A";
    // Convert 24-hour format to 12-hour format
    if (alarm_hour == 0)
    {
      alarm_hour_temp = 12; // Midnight is 12 AM
    }
    else if (alarm_hour > 12)
    {
      alarm_hour_temp = my_hour - 12; // Convert PM times
    }
    // Use fixed-size character array instead of String
    char dispString[6]; // "A1234\0" or "P1234\0"
    snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], alarm_hour_temp, alarm_min);
    sendDataViaI2C(dispString, '1');
  }
  delay(100);
  rtc.disableAlarm(1);
  rtc.clearAlarm(1);

  rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
  getTime()
      beep();
  delay(200);
  return 0;
}

void setup()
{
  FastGPIO::Pin<4>::setInput();    // the number of the pushbutton pin
  FastGPIO::Pin<5>::setInput();    // the number of the pushbutton pin
  FastGPIO::Pin<6>::setInput();    // the number of the pushbutton pin
  FastGPIO::Pin<7>::setInput();    // the number of the pushbutton pin
  FastGPIO::Pin<13>::setOutput(0); // pin for Buzzer output
  Wire.begin();                    // Join I2C bus as master
  Serial.begin(115200);
  Serial.println("Master ready");
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(alarmStopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(alarmStopPin), stopAlarm, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin), beep, FALLING);
  Serial.println(F("setup(): begin"));
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  // Disable and clear both alarms
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  rtc.writeSqwPinMode(DS3231_OFF); // Place SQW pin into alarm interrupt mode
  isAlarmActive = false;
  DateTime now = rtc.now(); // Get current time

  // Print current time and date
  char buff[] = "Start time is hh:mm:ss DDD, DD MMM YYYY";
  Serial.println(now.toString(buff));

  // Set alarm time
  rtc.setAlarm1(now + TimeSpan(0, 0, 0, 10), DS3231_A1_Second); // In 10 seconds time
  // rtc.setAlarm1(DateTime(2020, 6, 25, 15, 0, 0), DS3231_A1_Hour); // Or can be set explicity
  clearAll();
  Serial.println(F("setup(): ready"));
}

void loop()
{
  getTime();
  if (isAlarmActive)
  {
    beep();
  }
}
