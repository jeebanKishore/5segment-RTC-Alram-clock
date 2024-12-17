/*
  ===========================================================================
   Using the alarm on a DS3231 with the Adafruit RTClib library and SQW pin
  ===========================================================================
  
  Set an alarm on the DS3231 RTC module and monitor its status using the SQW
  to pin. Example sets alarm to trigger in 10 seconds after boot and then
  resets to a further 10 seconds after each trigger.
  
  
  Connections
  -----------
  SDA -> Arduino Analog (SDA pin)
  SCL -> Arduino Analog (SCL pin)
  VCC -> Arduino 5V
  GND -> Arduino GND
  SQW -> Arduino D4
*/
#include <stdint.h>
#include <Arduino.h>
#include <AceButton.h>
#include "RTClib.h"
RTC_DS3231 rtc;
int dot_status = 0;
// Pin Definitions
#define SEGMENT_MASK 0b11111100  // Mask for segments a-f on PORTD (Pins 2-7)
#define ANODE_MASK 0b00111100    // Mask for anodes on PORTB (Pins 10-13 as PB2-PB5)
#define SEGMENT_G 0b00000001     // Segment g on PB0
#define DOT 0b00000010           // DP on PB1
#define ANODE_5_BIT 0b00000001   // Anode for digit 5 on PC0
using ace_button::AceButton;
using ace_button::ButtonConfig;
using ace_button::LadderButtonConfig;
boolean isMenuActive = 0;
boolean isSetTimeActive = 0;
boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0;
int switch_status = 0;
int delay_time = 3;
int buzzer_pin = A1;
int buzzer_status = 0;
int beep_delay = 20;
int default_alarm_hour = 00, default_alarm_min = 00;
const int alarmPin = A3;  // The number of the pin for monitor alarm status on DS3231




//-----------------------------------------------------------------------------
// Configure AceButton
//-----------------------------------------------------------------------------

// The ADC pin used by the resistor ladder. Change to A6 pin on actual hardware
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
static AceButton* const BUTTONS[NUM_BUTTONS] = {
  &b0,
  &b1,
  &b2,
  &b3,
  &b4,
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
  819 /* 82%, 47 kohm */,
  1023 /* 100%, open circuit */,
};

// The LadderButtonConfig constructor binds the AceButton objects in the BUTTONS
// array to the LadderButtonConfig.
static LadderButtonConfig buttonConfig(
  BUTTON_PIN, NUM_LEVELS, LEVELS, NUM_BUTTONS, BUTTONS);

// The event handler for the buttons.
void handleEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  // Control the Buzzer only for Pressed and Released events.
  // If the MCU is rebooted while the button is pressed down, no event is triggered.
  switch (eventType) {
    case AceButton::kEventPressed:
      digitalWrite(buzzer_pin, HIGH);
      break;
    case AceButton::kEventReleased:
      digitalWrite(buzzer_pin, LOW);
      break;
  }
  if (AceButton::kEventClicked) {
    uint8_t pin = button->getPin();
    Serial.println(pin);
    // Reset all switch statuses
    switch_status = sw1_status = sw2_status = sw3_status = sw4_status = 0;

    // Set the corresponding switch status based on the clicked button
    switch (pin) {
      case 1:
        {
          sw1_status = 1;
          if (!isMenuActive) {
            isMenuActive = 1;
            menu(pin);
          } else menu(pin);
          break;
        }
      case 2:
        {
          sw2_status = 1;
          if (!isMenuActive) displayDate();
          else menu(pin);
          break;
        }
      case 3:
        {
          sw3_status = 1;
          if (!isMenuActive) displayTemperature();
          menu(pin);
          break;
        }
      case 4:
        {
          sw4_status = 1;
          if (!isMenuActive) displayAlarm(1);
          menu(pin);
          break;
        }
      default:
        // No action needed if not button 1-4
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
  if (!isMenuActive) {
    getTime();
  }
}



//-----------------------------------------------------------------------------

void beep() {
  digitalWrite(buzzer_pin, HIGH);
  delay(beep_delay);
  digitalWrite(buzzer_pin, LOW);
}




// Clear all segments and anodes
void clearAll() {
  PORTD &= ~SEGMENT_MASK;  // Clear segments (a-f)
  PORTB &= ANODE_MASK;     // Clear anodes, segment g, and DP
  PORTC &= ANODE_5_BIT;    // Clear anode for digit 5
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
    default: return 0b00000000;  // Blank
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


void loopDisplay(const char* charSetData, bool dot = false) {
    static const char* currentCharSet = nullptr;  // Pointer to the current character array
    static size_t currentDigit = 0;              // Digit currently being displayed (reverse order)
    static unsigned long lastUpdate = 0;         // Timestamp of the last display update
    const unsigned long displayInterval = 1;  // Time in miliseconds for each digit
    const size_t fixedLength = 5;                // Length of the fixed part (excluding '\n')

    // Check for a new character array
    if (charSetData != currentCharSet) {
        currentCharSet = charSetData;
        currentDigit = 0;  // Reset to the first digit in reverse order
    }

    // Update the display only if the interval has elapsed
    if (millis() - lastUpdate >= displayInterval) {
        lastUpdate = millis();

        // Clear the display and show the current digit
        clearAll();

        // Display in reverse order: from the last digit to the first
        if (currentDigit < fixedLength) {
            size_t reverseIndex = fixedLength - currentDigit - 1;  // Correct reverse index calculation
            char characterToDisplay = currentCharSet[reverseIndex];
            displayCharacter(currentDigit + 1, characterToDisplay, dot);
        }

        // Move to the next digit (loop back after completing all)
        currentDigit = (currentDigit + 1) % fixedLength;
    }
}


void menu(int switchPin) {
  Serial.println(switchPin);
  Serial.println(switch_status);
  Serial.println(sw1_status);
  Serial.println(sw2_status);
  Serial.println(sw3_status);
  Serial.println(sw4_status);
  loopDisplay("A8888", true);
}

void displayDate() {
  long t = 0;
  DateTime now = rtc.now();
  int my_date = now.day();
  int my_month = now.month();
  int my_year = now.year();  // Get the full 4-digit year

  // Create a character array to hold the formatted date string
  char dateString[6];  // Enough space for "dDDMM\0"

  // Format the date as "dDDMM"
  sprintf(dateString, "d%02d%02d", my_date, my_month);

  // Display the date for 5 seconds
  t = millis();
  while ((millis() - t) <= 5000) {
    loopDisplay(dateString, true);  // Send the formatted date string to display
  }

  // Create a character array for the year with prefix 'E'
  char yearString[6];  // Enough space for "Eyyyy\0"

  // Format the year as "Yyyyy"
  sprintf(yearString, "Y%04d", my_year);

  // Display the year for 0.5 seconds
  t = millis();
  while ((millis() - t) <= 5000) {
    loopDisplay(yearString, true);  // Send the formatted year string to display
  }
  return;
}

void displayTemperature() {
  char dispString[6];  // Enough space for "txxxx\0"

  // Convert the temperature to an integer representation
  int tempInt = static_cast<int>(rtc.getTemperature() * 100);  // Convert to integer (e.g., 26.54 -> 2654)

  // Format the string as "txxxx" where xxxx is the temperature
  sprintf(dispString, "t%04d", tempInt);


  long t = millis();
  while ((millis() - t) <= 5000) {  // Display for 5 seconds
    // Display the string
    loopDisplay(dispString, true);  // Call the function to display the formatted string
  }
  return;
}

void displayAlarm(int alarm) {
  char alarmDateString[6];                                                // Enough space for "A1234\0" or "P1234\0"
  DateTime alarmTime = (alarm == 1) ? rtc.getAlarm1() : rtc.getAlarm2();  // Get the stored alarm value
  int my_hour = alarmTime.hour();                                         // Get the hour
  int my_min = alarmTime.minute();                                        // Get the minute

  // Determine AM/PM and adjust hour
  char apDigit = (my_hour >= 12) ? 'P' : 'A';         // 'P' for PM, 'A' for AM
  my_hour = (my_hour > 12) ? my_hour - 12 : my_hour;  // Convert to 12-hour format

  // Format the alarm time as "A1234" or "P1234"
  snprintf(alarmDateString, sizeof(alarmDateString), "%c%02d%02d", apDigit, my_hour, my_min);

  // Display the alarm time for 1.5 seconds
  long startTime = millis();  // Declare and initialize the timer
  while ((millis() - startTime) <= 5000) {
    loopDisplay(alarmDateString, true);  // Send the formatted string to the display
  }
  return;
}

void getTime() {
  DateTime now = rtc.now();
  int my_hour = now.hour();
  int my_min = now.minute();
  const char* apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";
  // Convert 24-hour format to 12-hour format
    if (my_hour == 0) {
        my_hour = 12;  // Midnight is 12 AM
    } else if (my_hour > 12) {
        my_hour -= 12;  // Convert PM times
    }
  // Use fixed-size character array instead of String
  char dispString[6];  // "A1234\0" or "P1234\0"
  snprintf(dispString, sizeof(dispString), "%c%02d%02d", apDigit[0], my_hour, my_min);
  loopDisplay(dispString, true);
}

//--------------------------------------------------------------------------
void setup() {
  // Configure PORTD (segments a-f) as output
  DDRD |= SEGMENT_MASK;
  // Configure PORTB (segments g, DP, and anodes) as output
  DDRB |= ANODE_MASK | SEGMENT_G | DOT;
  // Configure PORTC (anode for digit 5) as output
  DDRC |= ANODE_5_BIT;
  // Clear all outputs
  pinMode(alarmPin, INPUT_PULLUP);
  pinMode(buzzer_pin, OUTPUT);
  const char* charSetData = "88888";
  loopDisplay(charSetData);
  Serial.begin(115200);
  pinMode(alarmPin, INPUT_PULLUP);  // Set alarm pin as pullup
  Serial.println(F("setup(): begin"));
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  // Initialize built-in LED as an output, and start with LED off.
  //pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, LED_OFF);

  // Don't use internal pull-up resistor because it will change the effective
  // resistance of the resistor ladder.
  pinMode(BUTTON_PIN, INPUT);

  // Configure the ButtonConfig with the event handler, and enable all higher
  // level events.
  buttonConfig.setEventHandler(handleEvent);
  buttonConfig.setFeature(ButtonConfig::kFeatureClick);
  // buttonConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
  // buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  // buttonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  // If required set to to compile time
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // Or to an explicit date & time, e.g. July 3, 2020 at 8pm
  //rtc.adjust(DateTime(2020, 7, 3, 20, 0, 0));

  // Disable and clear both alarms
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  rtc.writeSqwPinMode(DS3231_OFF);  // Place SQW pin into alarm interrupt mode

  DateTime now = rtc.now();  // Get current time

  // Print current time and date
  char buff[] = "Start time is hh:mm:ss DDD, DD MMM YYYY";
  Serial.println(now.toString(buff));

  // Set alarm time
  rtc.setAlarm1(now + TimeSpan(0, 0, 0, 10), DS3231_A1_Second);  // In 10 seconds time
  //rtc.setAlarm1(DateTime(2020, 6, 25, 15, 0, 0), DS3231_A1_Hour); // Or can be set explicity
  clearAll();
  Serial.println(F("setup(): ready"));
}

void loop() {
  // Check if SQW pin shows alarm has fired
  if (digitalRead(alarmPin) == LOW) {
    // The alarm has just fired

    DateTime now = rtc.now();
    //beep();
    // Disable and clear alarm
    rtc.disableAlarm(1);
    rtc.clearAlarm(1);

    // Perhaps reset to new time if required
    rtc.setAlarm1(now + TimeSpan(0, 0, 0, 10), DS3231_A1_Second);  // Set for another 10 seconds
  }
  checkButtons();
  // getTime();
}