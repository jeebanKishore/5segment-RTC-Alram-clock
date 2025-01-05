/*
 * A demo of LadderButtonConfig for buttons connected to a single pin
 * using a resistor ladder.
 */
#include <Arduino.h>
#include <Wire.h>
#include <string.h>
#include <AceButton.h>
#include "RTClib.h"

#define SNOOZE_DURATION 300000                          // Snooze duration in milliseconds (e.g., 5 minutes)
const unsigned long ALARM_DURATION = 20UL * 60 * 1000;  // 20 minutes in milliseconds
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

using ace_button::AceButton;
using ace_button::ButtonConfig;
using ace_button::LadderButtonConfig;
volatile boolean sw1_status = 0, sw2_status = 0, sw3_status = 0, sw4_status = 0;
volatile boolean isMenuActive = 0;

uint8_t buzzerPin = A1;
uint8_t alramTriggerPin = A3;




volatile boolean isAlarmActive = false;     // Flag to check if the alarm is active.
volatile unsigned long alarmStartTime = 0;  // When the alram started from
volatile boolean alarmTriggered = false;    // New flag for alarm activation
volatile boolean snoozeflag = 0;


uint16_t my_year = 0;
uint8_t my_month = 0, my_date = 0, my_hour = 0, my_min = 0, alarm_hour = 0, alarm_min = 0;

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
void handleEvent(AceButton *button, uint8_t eventType, uint8_t /*buttonState*/) {
  switch (eventType) {
    case AceButton::kEventPressed:
      {
        uint8_t pin = button->getPin();
        switch (pin) {
          case 1:
            sw1_status = 1;
            if (isMenuActive == 0 && isAlarmActive == 0) {
              isMenuActive = 1;
              displayMenu();
            }
            break;
          case 2:
            sw2_status = 1;
            if (isMenuActive == 0 && isAlarmActive == 0) displayDate();
            break;
          case 3:
            sw3_status = 1;
            if (isMenuActive == 0 && isAlarmActive == 0) displayTemperature();
            break;
          case 4:
            sw4_status = 1;
            if (isMenuActive == 0 && isAlarmActive == 0) displayAlarm();
            break;
        }
        break;
      }
    case AceButton::kEventReleased:
      {
        sw1_status = 0;
        sw2_status = 0;
        sw3_status = 0;
        sw4_status = 0;
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
    // Check the state of each button
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
      AceButton *button = BUTTONS[i];
      uint8_t pin = button->getPin();
      bool isPressed = button->isPressedRaw();

      // Print the status of each switch
      switch (pin) {
        case 1:
          sw1_status = isPressed ? 1 : 0;
          break;
        case 2:
          sw2_status = isPressed ? 1 : 0;
          break;
        case 3:
          sw3_status = isPressed ? 1 : 0;
          break;
        case 4:
          sw4_status = isPressed ? 1 : 0;
          break;
        default:
          {
            sw1_status = 0;
            sw2_status = 0;
            sw3_status = 0;
            sw4_status = 0;
            break;
          }
      }
    }
  }
}

//-----------------------------------------------------------------------------

void beep(uint8_t initialBeepDelay = 20) {
  if (initialBeepDelay == 0) {
    digitalWrite(buzzerPin, LOW);  // No beep sound
    return;
  }
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
    digitalWrite(buzzerPin, HIGH);
    delay(initialBeepDelay);
    digitalWrite(buzzerPin, LOW);
  }
}
// void beep(bool continuous = false, uint8_t initialBeepDelay = 20) {
//   if (continuous) {
//     if (isAlarmActive) {
//       unsigned long currentTime = millis();
//       if (currentTime - lastBeepTime >= currentBeepDelay) {
//         beepState = !beepState;  // Toggle beep state
//         digitalWrite(buzzerPin, beepState ? HIGH : LOW);
//         lastBeepTime = currentTime;

//         // Gradually decrease the delay, but not below 300ms
//         if (beepState && currentBeepDelay > 300) {
//           currentBeepDelay -= 50;                              // Decrease the delay by 50ms each iteration
//           if (currentBeepDelay < 300) currentBeepDelay = 300;  // Cap at 300ms
//         }
//       }
//     } else {
//       digitalWrite(buzzerPin, LOW);  // Turn off beep when alarm is not active
//     }
//   } else {
//     digitalWrite(buzzerPin, HIGH);
//     delay(initialBeepDelay);
//     digitalWrite(buzzerPin, LOW);
//   }
// }
// Clear all segments and anodes
void clearAll() {
  PORTD &= ~SEGMENT_MASK;  // Clear segments (a-f)
  PORTB &= ANODE_MASK;     // Clear anodes, segment g, and DP
  PORTC &= ANODE_5_BIT;    // Clear anode for digit 5
}

// Function to send data repeatedly for a specified duration
void sendDataRepeatedly(const char *data, uint16_t timeFrame = 5000) {
  long startTime = millis();
  // Check if data is not empty
  if (data != nullptr && strlen(data) > 0) {
    memcpy((char *)buf, data, 6);
  }
  while ((millis() - startTime) <= timeFrame) {
    delay(1);
    loopDisplay();
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
    case 'O': return 0b01011100;
    case 'Y': return 0b01110010;
    case 'H': return 0b01110110;
    case 'F': return 0b01110001;
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

void loopDisplay() {
  char displayData[6] = { 0 };

  // Copy the first 5 characters from the buffer
  memcpy(displayData, (const char *)buf, 5);
  displayData[5] = '\0';  // Ensure null-termination

  unsigned long currentMillis = millis();
  bool dot_status = false;

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
  }

  // Display on the 7-segment
  for (int i = 4; i >= 0; i--) {
    displayCharacter(5 - i, displayData[i], dot_status);
    delay(1);
  }
  clearAll();
}

// Updaate Gloal time details
uint8_t updateGlobalTimeVars() {
  DateTime now = rtc.now();
  my_hour = now.hour();
  my_min = now.minute();
  my_date = now.day();
  my_month = now.month();
  my_year = now.year();
  DateTime alram1 = rtc.getAlarm1();
  alarm_hour = alram1.hour();
  alarm_min = alram1.minute();
  return 0;
}

// Function to display the current date
uint8_t displayDate() {
  beep();
  updateGlobalTimeVars();
  sprintf((char *)buf, "d%02d%02d1", my_date, my_month);
  sendDataRepeatedly((char *)buf);
  beep();
  // char yearString[6];
  sprintf((char *)buf, "Y%04d0", my_year);
  sendDataRepeatedly((char *)buf);
  return 0;
}

bool isAlarm1Set() {
  Wire.beginTransmission(0x68);  // DS3231 I2C address
  Wire.write(0x0E);              // Control register address
  Wire.endTransmission();
  Wire.requestFrom(0x68, 1);
  uint8_t control = Wire.read();

  // Check if A1IE (Alarm 1 Interrupt Enable) bit is set
  return (control & 0b00000001);
}

void displayAlarm() {
  beep();
  if (!isAlarm1Set()) {
    sendDataRepeatedly("ALOFF0", 2000);
  } else {
    char apDigit = (alarm_hour >= 12) ? 'P' : 'A';
    uint8_t alarm_hour_tmp = (alarm_hour == 0) ? 12 : (alarm_hour > 12) ? alarm_hour - 12
                                                                        : alarm_hour;
    snprintf((char *)buf, sizeof(buf), "%c%02d%02d1", apDigit, alarm_hour_tmp, alarm_min);
    sendDataRepeatedly((char *)buf);
  }
}

void displayTemperature() {
  beep();
  int tempInt = static_cast<int>(rtc.getTemperature() * 100);
  snprintf((char *)buf, sizeof(buf), "t%04d1", tempInt);
  sendDataRepeatedly((char *)buf);
}

// Function to get and display the current time
void getTime() {
  const char *apDigit = (my_hour >= 12) ? "P" : "A";
  uint8_t my_hour_tmp = (my_hour == 0) ? 12 : (my_hour > 12) ? my_hour - 12
                                                             : my_hour;
  snprintf((char *)buf, sizeof(buf), "%c%02d%02d2", apDigit[0], my_hour_tmp, my_min);
}

// Function to display the menu
uint8_t displayMenu() {
  beep();
  sendDataRepeatedly("Enu==1", 2000);
  delay(100);
  checkButtons();
  // Display an animation when entered into menu
  while (!readSwitchStatus()) {
    DateTime now = rtc.now();
    if (now.second() % 2) {
      sendDataRepeatedly("-----1", 2000);
    } else {
      sendDataRepeatedly("Enu==1", 2000);
    }
    checkButtons();  // Check for button events
  }

  // Wait for user input and handle button events
  while (true) {
    checkButtons();  // Continuously check for button events

    if (sw1_status) {
      // Set the clock parameters
      delay(100);
      if (setAlarm()) return 1;
      else if (setTime()) return 1;
      else if (setDate()) return 1;
      else if (setYear()) return 1;
    } else if (sw2_status || sw3_status || sw4_status) {
      // Handle other button events if needed
      isMenuActive = 0;
      beep();
      return 0;
    } else {
      // Handle other button events if needed
      isMenuActive = 0;
      beep();
      return 0;
    }
  }
}
// Function to set the alarm time
uint8_t setAlarm() {
  sendDataRepeatedly("5EtAL0", 2000);
  updateGlobalTimeVars();
  boolean alarm_changed_status = 0;
  readSwitchStatus();
  sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
    if (sw2_status) {
      beep();
      alarm_changed_status = 1;
      delay(150);
      alarm_hour++;
      if (alarm_hour > 23) alarm_hour = 0;
      sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
    } else if (sw3_status) {
      beep();
      alarm_changed_status = 1;
      delay(150);
      alarm_min++;
      if (alarm_min > 59) alarm_min = 0;
      sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
    } else if (sw4_status) {
      beep();
      if (alarm_changed_status) {
        rtc.clearAlarm(1);
        rtc.disableAlarm(1);
        rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);
        beep();
        delay(200);
        sendDataRepeatedly("AL5Et0", 2000);
        alarm_changed_status = 0;
        sendUpdatedTimeOrAlaramToDisplay(alarm_hour, alarm_min);
      } else {
        sendDataRepeatedly("ALOFF0", 2000);
        rtc.clearAlarm(1);
        rtc.disableAlarm(1);
      }
      isMenuActive = 0;
      return 1;
    }
  }

  delay(100);
  updateGlobalTimeVars();
  return 0;
}


// Function to set the current time
uint8_t setTime() {
  sendDataRepeatedly("5ttIE0", 2000);
  updateGlobalTimeVars();
  boolean time_changed_status = 0;
  readSwitchStatus();
  sendUpdatedTimeOrAlaramToDisplay(my_hour, my_min);
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    sendUpdatedTimeOrAlaramToDisplay(my_hour, my_min);
    if (sw2_status) {
      beep();
      time_changed_status = 1;
      delay(150);
      my_hour++;
      if (my_hour > 23) my_hour = 0;
      sendUpdatedTimeOrAlaramToDisplay(my_hour, my_min);
    } else if (sw3_status) {
      beep();
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
      isMenuActive = 0;
      return 1;
    }
  }
  updateGlobalTimeVars();
  delay(200);
  return 0;
}

uint8_t sendUpdatedTimeOrAlaramToDisplay(uint8_t my_hour, uint8_t my_min) {
  uint8_t my_hour_tmp = (my_hour == 0) ? 12 : (my_hour > 12) ? my_hour - 12
                                                             : my_hour;
  const char *apDigit = (my_hour > 12 && my_min > 0) ? "P" : "A";
  // char dispString[7];
  snprintf((char *)buf, sizeof(buf), "%c%02d%02d1", apDigit[0], my_hour_tmp, my_min);
  loopDisplay();
  return 0;
}

// Function to set the current date
uint8_t setDate() {
  sendDataRepeatedly("5tdtE0", 5000);
  updateGlobalTimeVars();
  boolean date_changed_status = 0;
  readSwitchStatus();
  sendUpdatedDateToDisplay(my_date, my_month);
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    sendUpdatedDateToDisplay(my_date, my_month);
    if (sw2_status) {
      beep();
      date_changed_status = 1;
      delay(150);
      my_date++;
      if (my_date > 31) my_date = 1;
      sendUpdatedDateToDisplay(my_date, my_month);
    } else if (sw3_status) {
      beep();
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
      isMenuActive = 0;
      return 1;
    }
  }
  updateGlobalTimeVars();
  delay(200);
  return 0;
}

uint8_t sendUpdatedDateToDisplay(uint8_t my_date, uint8_t my_month) {
  sprintf((char *)buf, "d%02d%02d1", my_date, my_month);
  loopDisplay();

  return 0;
}

// Function to set the current year
uint8_t setYear() {
  sendDataRepeatedly("5tYEA0", 5000);
  updateGlobalTimeVars();
  boolean year_changed_status = 0;
  readSwitchStatus();
  // Serial.println("Year set start");
  // Serial.println(my_year);
  setUpdatedYearToDisplay(my_year);
  beep();
  while (!sw1_status) {
    readSwitchStatus();
    setUpdatedYearToDisplay(my_year);
    if (sw2_status) {
      beep();
      year_changed_status = 1;
      delay(150);
      my_year--;
      if (my_year < 2025) my_year = 2025;
      setUpdatedYearToDisplay(my_year);
    } else if (sw3_status) {
      beep();
      year_changed_status = 1;
      delay(150);
      my_year++;
      if (my_year > 2070) my_year = 2070;
      setUpdatedYearToDisplay(my_year);
    } else if (sw4_status) {
      beep();
      setUpdatedYearToDisplay(my_year);
      if (year_changed_status) {
        // Serial.println("adter button press");
        // Serial.println(my_year);
        // Serial.println(my_month);
        // Serial.println(my_date);
        // Serial.println(my_hour);
        // Serial.println(my_min);
        // Serial.println("****************");

        uint8_t year = (my_year - 2000U) < 25U ? 25U : (my_year - 2000U);

        writeOnAddress(year, 0x06);
        //rtc.adjust(DateTime(my_year, my_month, my_date, my_hour, my_min, 0));
        beep();
        delay(50);

        sendDataRepeatedly("YE5Et0", 5000);
        year_changed_status = 0;
        setUpdatedYearToDisplay(my_year);
      }
      isMenuActive = 0;
      return 1;
    }
  }



  updateGlobalTimeVars();
  delay(200);
  return 0;
}

uint8_t setUpdatedYearToDisplay(uint16_t my_year) {
  sprintf((char *)buf, "Y%04d0", my_year);
  loopDisplay();
  return 0;
}

void writeOnAddress(byte value, int address) {
  Wire.beginTransmission(0x68);
  Wire.write(address);
  Wire.write(decToBcd(value));
  Wire.endTransmission();
}

byte decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}


//Function to read the status of each switch with debounce
boolean readSwitchStatus() {
  checkButtons();
  // Return true if any switch is active
  return (sw1_status || sw2_status || sw3_status || sw4_status);
}




// Function to stop the alarm completely
uint8_t stopAlarm() {
  Serial.println(F("RTC Alarm stopping...."));
  if (!isAlarmActive) return 0;
  Serial.println(F("Alarm stopped"));
  isAlarmActive = false;
  snoozeflag = false;
  rtc.clearAlarm(1);                                                           // Clear any active alarm flags
  rtc.setAlarm1(DateTime(0, 0, 0, alarm_hour, alarm_min, 0), DS3231_A1_Hour);  // Set Alram for next day
  sendDataRepeatedly("ALOFF0", 5000);                                          // Notify the display
  beep();                                                                      // Signal that the alarm is stopped
  return 0;
}

// Function to snooze the alarm for 5 minutes
uint8_t snoozeAlarm() {
  if (!isAlarmActive) return 0;
  Serial.println(F("Alarm snoozed for 5 minutes"));
  isAlarmActive = false;                        // Temporarily disable the alarm
  alarmStartTime = millis() + SNOOZE_DURATION;  // Set snooze duration
  sendDataRepeatedly("SnOOZ0", 2000);           // Notify the display
  beep();                                       // Signal snooze activation
  return 0;
}

void setup() {
  // Configure PORTD (segments a-f) as output
  DDRD |= SEGMENT_MASK;
  // Configure PORTB (segments g, DP, and anodes) as output
  DDRB |= ANODE_MASK | SEGMENT_G | DOT;
  // Configure PORTC (anode for digit 5) as output
  DDRC |= ANODE_5_BIT;
  Serial.begin(115200);
  Serial.println(F("setup(): begin"));

  pinMode(buzzerPin, OUTPUT);
  pinMode(alramTriggerPin, INPUT_PULLUP);
  digitalWrite(buzzerPin, LOW);

  // Don't use internal pull-up resistor because it will change the effective
  // resistance of the resistor ladder.
  pinMode(BUTTON_PIN, INPUT);

  // Configure the ButtonConfig with the event handler, and enable all higher
  // level events.
  buttonConfig.setEventHandler(handleEvent);
  buttonConfig.setFeature(ButtonConfig::kFeatureClick);
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    Serial.flush();
    abort();
  }

  if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, setting the time!"));
    // Set the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  } else {
    Serial.println(F("RTC is running with correct time."));
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
  uint8_t pinState = digitalRead(alramTriggerPin);
  checkButtons();
  updateGlobalTimeVars();

  getTime();


  if (pinState == LOW) {
    if (rtc.alarmFired(1) == true) {  // Check if the alarm is triggered
      Serial.println(F("RTC Alarm triggered"));
      isAlarmActive = true;
      alarmStartTime = millis();  // Record the alarm start time
      currentBeepDelay = 20;      // Reset beep delay
    }
  }


  // Handle active alarm
  if (isAlarmActive == true) {
    rtc.clearAlarm(1);  // Clear the alarm
    checkButtons();
    // Check switch inputs
    if (sw4_status) {
      // Stop the alarm
      Serial.println(F("RTC Alarm stopping"));
      stopAlarm();
      return;
    } else if (sw1_status || sw2_status || sw3_status) {
      // Snooze the alarm for 5 minutes
      snoozeflag = true;
      snoozeAlarm();
      return;
    }

    // Check if 20 minutes have elapsed since the alarm started
    if (millis() - alarmStartTime >= ALARM_DURATION) {
      Serial.println(F("Alarm auto-stopped after 20 minutes"));
      stopAlarm();
      return;
    }

    // Continue alarm sound
    beep(currentBeepDelay);  // Ensure beep is non-blocking
  }

  // Handle snoozed alarm
  if (!isAlarmActive && snoozeflag && millis() >= alarmStartTime) {
    Serial.println(F("Snoozed alarm reactivated"));
    isAlarmActive = true;
    alarmStartTime = millis();  // Reset the alarm start time
  }
  loopDisplay();
  beep(isAlarmActive ? currentBeepDelay : 0);
  delay(1);
}