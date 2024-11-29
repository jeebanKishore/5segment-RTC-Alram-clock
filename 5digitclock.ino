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

int my_hour = 0, my_min = 0, my_date = 0, my_month = 0, my_second = 0, my_year;
// boolean sw_status[4] = {0};  // Array for switch statuses
boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0;
int switch_status = 0;
int delay_time = 3;
int buzzer_pin = A1;
int buzzer_status = 0;
int beep_delay = 20;
int default_alarm_hour = 19, default_alarm_min = 00;
const int alarmPin = A2;  // The number of the pin for monitor alarm status on DS3231
boolean alarm_status = 1;

//-----------------------------------------------------------------------------
// Configure AceButton
//-----------------------------------------------------------------------------

// The ADC pin used by the resistor ladder.
static const uint8_t BUTTON_PIN = A6;

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
  510 /* 0%, 1st switch */,
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

  // Print out a message for all events.
  Serial.print(F("handleEvent(): "));
  Serial.print(F("virtualPin: "));
  Serial.print(button->getPin());
  Serial.print(F("; eventType: "));
  Serial.print(AceButton::eventName(eventType));
  Serial.print(F("; buttonState: "));
  Serial.println(buttonState);

  // Control the LED only for the Pressed and Released events.
  // Notice that if the MCU is rebooted while the button is pressed down, no
  // event is triggered and the LED remains off.
  // switch (eventType) {
  //   case AceButton::kEventPressed:
  //     digitalWrite(LED_PIN, LED_ON);
  //     break;
  //   case AceButton::kEventReleased:
  //     digitalWrite(LED_PIN, LED_OFF);
  //     break;
  // }
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

void get_time_date() {
  DateTime now = rtc.now();
  my_hour = now.hour();
  my_min = now.minute();
  my_date = now.day();
  my_month = now.month();
  my_second = now.second();
}

String int_to_string(int i1, int i2) {
  char my_display[2];  // Enough space for "00:00\0"

  // Format the integers into the character array
  if (i1 < 10 && i2 < 10) {
    sprintf(my_display, "0%d:0%d", i1, i2);
  } else if (i1 < 10 && i2 >= 10) {
    sprintf(my_display, "0%d:%d", i1, i2);
  } else if (i1 >= 10 && i2 < 10) {
    sprintf(my_display, "%d:0%d", i1, i2);
  } else {
    sprintf(my_display, "%d:%d", i1, i2);
  }

  return String(my_display);  // Return as a String object
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

// Display characters in a string
void loopDisplay(const char* charSetData, bool dot = false) {
  for (size_t i = 0; charSetData[i] != '\0'; i++) {
    displayCharacter(i, charSetData[i], dot);
    delay(1);
  }
}

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
  while (!Serial)
    ;  // Wait until Serial is ready - Leonardo/Micro
  Serial.println(F("setup(): begin"));
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  DateTime now = rtc.now();
  my_year = now.year();
  // Disable and clear both alarms
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  rtc.writeSqwPinMode(DS3231_OFF);  // Place SQW pin into alarm interrupt mode



  // Don't use internal pull-up resistor because it will change the effective
  // resistance of the resistor ladder.
  pinMode(BUTTON_PIN, INPUT);

  // Configure the ButtonConfig with the event handler, and enable all higher
  // level events.
  buttonConfig.setEventHandler(handleEvent);
  buttonConfig.setFeature(ButtonConfig::kFeatureClick);
  buttonConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureRepeatPress);
  clearAll();
  Serial.println(F("setup(): ready"));
}

void loop() {
  DateTime now = rtc.now();
  checkButtons();
  my_hour = now.hour();
  const char* apDigit = (my_hour > 12) ? "P" : "A";
  my_hour = (my_hour > 12) ? my_hour - 12 : my_hour;
  my_min = now.minute();

  // Use fixed-size character array instead of String
  char dispString[6];  // "A12:34\0" or "P12:34\0"
  snprintf(dispString, sizeof(dispString), "%c%02d:%02d", apDigit[0], my_hour, my_min);
  Serial.println(apDigit);
  Serial.println(my_hour);
  Serial.println(my_min);
  loopDisplay(dispString, true);

  // Check if SQW pin shows alarm has fired
  if (digitalRead(alarmPin) == LOW) {
    char buff[50];
    Serial.println(now.toString(buff));
    rtc.disableAlarm(1);
    rtc.clearAlarm(1);
  }
}
