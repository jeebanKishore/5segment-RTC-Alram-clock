/*
 * A demo of LadderButtonConfig for buttons connected to a single pin
 * using a resistor ladder.
 */

#include <stdint.h>
#include <Arduino.h>
#include <AceButton.h>

#include "RTClib.h"
#define SNOOZE_DURATION 300000  // Snooze duration in milliseconds (e.g., 5 minutes)

// Pin Definitions
#define SEGMENT_MASK 0b11111100   // Mask for segments a-f on PORTD (Pins 2-7)
#define ANODE_MASK 0b00111100     // Mask for anodes on PORTB (Pins 10-13 as PB2-PB5)
#define SEGMENT_G 0b00000001      // Segment g on PB0
#define DOT 0b00000010            // DP on PB1
#define ANODE_5_BIT 0b00000001    // Anode for digit 5 on PC0
RTC_DS3231 rtc;                   // Instance of RTC_DS3231 to manage RTC operations.
volatile char buf[8] = "888880";  // Buffer for received data, initialized
unsigned long lastToggleTime = 0;
volatile bool blinkState = false;
volatile int dot_status = 0;
using ace_button::AceButton;
using ace_button::ButtonConfig;
using ace_button::LadderButtonConfig;
volatile boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0;
int switch_status = 0;

int buzzerPin = A1;




volatile boolean isAlarmActive = false;     // Flag to check if the alarm is active.
volatile unsigned long alarmStartTime = 0;  // When the alram started from
volatile boolean alarmTriggered = false;    // New flag for alarm activation
volatile boolean snoozeflag = 0;

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

//-----------------------------------------------------------------------------
// Configure AceButton
//-----------------------------------------------------------------------------

// The ADC pin used by the resistor ladder.
static const uint8_t BUTTON_PIN = A2;

// Create 4 AceButton objects, with their corresonding virtual pin numbers 0 to
// 3. Note that we could use an array of `AceButton BUTTONS[NUM_BUTTONS]`, and
// use a loop in setup() to initialize these buttons. But I think writing this
// out explicitly is easier to understand for demo purposes.
//
// We use the 4-parameter AceButton() constructor with the `buttonConfig`
// parameter set to `nullptr` to prevent the creation of the default
// SystemButtonConfig which will never be used. This saves about 30 bytes of
// flash and 26 bytes of static RAM on an AVR processor.
static const uint8_t NUM_BUTTONS = 5;
static AceButton b0(nullptr, 0);
static AceButton b1(nullptr, 1);
static AceButton b2(nullptr, 2);
static AceButton b3(nullptr, 3);
static AceButton b4(nullptr, 4);
// button 4 cannot be used because it represents "no button pressed"
static AceButton *const BUTTONS[NUM_BUTTONS] = {
  &b0, &b1, &b2, &b3, &b4
};

// Define the ADC voltage levels for each button. In this example, we want 4
// buttons, so we need 5 levels. Ideally, the voltage levels should correspond
// to 0%, 25%, 50%, 75%, 100%. We can get pretty close by using some common
// resistor values (4.7k, 10k, 47k). Use the examples/LadderButtonCalibrator
// program to double-check these calculated values.
static const uint8_t NUM_LEVELS = NUM_BUTTONS + 1;
static const uint16_t LEVELS[NUM_LEVELS] = {
  0 /* 0%, short to ground */,
  510 /* 0%, short to ground */,
  681 /* 32%, 4.7 kohm */,
  770 /* 50%, 10 kohm */,
  860 /* 82%, 47 kohm */,
  1023 /* 100%, open circuit */,
};

// The LadderButtonConfig constructor binds the AceButton objects in the BUTTONS
// array to the LadderButtonConfig.
static LadderButtonConfig buttonConfig(
  BUTTON_PIN, NUM_LEVELS, LEVELS, NUM_BUTTONS, BUTTONS);

// The event handler for the buttons.
void handleEvent(AceButton *button, uint8_t eventType,
                 uint8_t /*buttonState*/) {

  // Control the LED only for the Pressed and Released events.
  // Notice that if the MCU is rebooted while the button is pressed down, no
  // event is triggered and the LED remains off.
  switch (eventType) {
    case AceButton::kEventPressed:
      {
        uint8_t pin = button->getPin();
        digitalWrite(buzzerPin, HIGH);
        switch (pin) {
          case 1:
            {
              sw1_status = 1;
              break;
            }
          case 2:
            {
              sw2_status = 1;
              break;
            }
          case 3:
            {
              sw3_status = 1;
              break;
            }
          case 4:
            {
              sw4_status = 1;
              break;
            }
        }
        break;
      }
    case AceButton::kEventReleased:
      {
        digitalWrite(buzzerPin, LOW);
        sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0;
        break;
      }
  }
}

// On most processors, this should be called every 4-5ms or faster, if the
// default debouncing time is ~20ms. On a ESP8266, we must sample *no* faster
// than 4-5 ms to avoid disconnecting the WiFi connection. See
// https://github.com/esp8266/Arduino/issues/1634 and
// https://github.com/esp8266/Arduino/issues/5083. To be safe, let's rate-limit
// this on all processors to about 200 samples/second.
void checkButtons() {
  static uint16_t prev = millis();

  // DO NOT USE delay(5) to do this.
  // The (uint16_t) cast is required on 32-bit processors, harmless on 8-bit.
  uint16_t now = millis();
  if ((uint16_t)(now - prev) >= 5) {
    prev = now;
    buttonConfig.checkButtons();
  }
}

//-----------------------------------------------------------------------------

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
// Clear all segments and anodes
void clearAll() {
  PORTD &= ~SEGMENT_MASK;  // Clear segments (a-f)
  PORTB &= ANODE_MASK;     // Clear anodes, segment g, and DP
  PORTC &= ANODE_5_BIT;    // Clear anode for digit 5
}

// Function to send data via I2C
void sendDataViaI2C(const char *data) {
  if (strlen(data) != 6) {
    Serial.println("Error: Data must be exactly 6 characters long.");
    Serial.println(data);
    return;
  }
  strncpy(buf, data, 6);  // Copy the data to the buffer
  buf[6] = '\0';  // Ensure the buffer is null-terminated
}

// Function to send data repeatedly for a specified duration
void sendDataRepeatedly(const char *data, unsigned int timeFrame = 5000) {
  long startTime = millis();
  while ((millis() - startTime) <= timeFrame) {
    sendDataViaI2C(data);
    delay(1);
  }
}

// Get segment pattern for a character
uint8_t getSegmentPattern(char character) {
  switch (character) {
    case '0': return 0b00111111;
    case '1': return 0b00000110;
    case '2': return 0b01011011;
    case '3': return 0b01001111;
    case '4': return 0b01100110;
    case '5': return 0b01101101;
    case '6': return 0b01111101;
    case '7': return 0b00000111;
    case '8': return 0b01111111;
    case '9': return 0b01101111;
    case 'A': return 0b01110111;
    case 'P': return 0b01110011;
    case 'V': return 0b00111110;
    case 'D': return 0b01100011;
    case 'E': return 0b01111001;
    case 'u': return 0b00011100;
    case 'n': return 0b01010100;
    case 't': return 0b01111000;
    case 'd': return 0b01011110;
    case '-': return 0b01000000;
    case 'L': return 0b00111000;
    case 'I': return 0b00001110;
    case 'o': return 0b01100011;
    case 'Y': return 0b01110010;
    case 'H': return 0b01110110;
    case 'F': return 0b01110101;
    case 'e': return 0b01111011;
    case '=': return 0b00000000;  // Blank
    default: return 0b00000000;   // Blank
  }
}

// Activate a specific digit position
void activateDigit(uint8_t position) {
  PORTB |= ANODE_MASK;
  PORTC |= ANODE_5_BIT;
  if (position >= 1 && position <= 4) {
    PORTB &= ~(0b00000100 << (position - 1));  // PB2-PB5
  } else if (position == 5) {
    PORTC &= ~ANODE_5_BIT;  // PC0
  }
}

// Display a character on a specific digit
void displayCharacter(uint8_t position, char character, bool dot = false) {
  clearAll();
  uint8_t pattern = getSegmentPattern(character);
  PORTD = (PORTD & ~SEGMENT_MASK) | ((pattern & 0b01111111) << 2);
  PORTB = (PORTB & ~SEGMENT_G) | (pattern >> 6);
  PORTB = dot ? (PORTB | DOT) : (PORTB & ~DOT);
  activateDigit(position);
}

void loopDisplay(const char *charSetData, bool dot = false) {
  size_t length = strlen(charSetData);  // Use strlen to calculate the length

  // Iterate in reverse order
  for (size_t i = length; i > 0; i--) {
    displayCharacter(length - i + 1, charSetData[i - 1], dot);
    delay(1);
  }
  clearAll();
}

void displayMe() {
  char displayData[6] = { 0 };

  // Copy the first 5 characters from the buffer
  strncpy(displayData, (const char *)buf, 5);
  displayData[5] = '\0';  // Ensure null-termination
  unsigned long currentMillis = millis();
  // Determine dot status
  switch (buf[5]) {
    case '0':
      dot_status = false;
      break;
    case '1':
      dot_status = true;
      break;
    case '2':
      // Blink the dot every 500ms

      if ((currentMillis - lastToggleTime) >= 500) {
        blinkState = !blinkState;
        lastToggleTime = currentMillis;
      }
      dot_status = blinkState;
      break;
    default:
      dot_status = false;
      break;
  }

  // Display on the 7-segment
  loopDisplay(displayData, dot_status);
}


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
  // Return true if any switch is active
  return (sw1_status || sw2_status || sw3_status || sw4_status);
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

void setup() {
  delay(1000);  // some microcontrollers reboot twice
  // Configure PORTD (segments a-f) as output
  DDRD |= SEGMENT_MASK;
  // Configure PORTB (segments g, DP, and anodes) as output
  DDRB |= ANODE_MASK | SEGMENT_G | DOT;
  // Configure PORTC (anode for digit 5) as output
  DDRC |= ANODE_5_BIT;
  Serial.begin(115200);
  while (!Serial)
    ;  // Wait until Serial is ready - Leonardo/Micro
  Serial.println(F("setup(): begin"));

  // Initialize built-in LED as an output, and start with LED off.
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // Don't use internal pull-up resistor because it will change the effective
  // resistance of the resistor ladder.
  pinMode(BUTTON_PIN, INPUT);

  // Configure the ButtonConfig with the event handler, and enable all higher
  // level events.
  buttonConfig.setEventHandler(handleEvent);
  buttonConfig.setFeature(ButtonConfig::kFeatureClick);
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
  checkButtons();
  updateGlobalTimeVars();
  checkSwitch();
  getTime();



  if (rtc.alarmFired(1)) {  // Check if the alarm is triggered
    Serial.println(F("RTC Alarm triggered"));
    isAlarmActive = true;
    alarmStartTime = millis();  // Record the alarm start time
    currentBeepDelay = 20;      // Reset beep delay
  } else {
    Serial.println(F("False interrupt detected"));
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
  displayMe();
  delay(1);
}