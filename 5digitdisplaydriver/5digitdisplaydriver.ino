int dot_status = 0;
// Pin Definitions
#define SEGMENT_MASK 0b11111100  // Mask for segments a-f on PORTD (Pins 2-7)
#define ANODE_MASK 0b00111100    // Mask for anodes on PORTB (Pins 10-13 as PB2-PB5)
#define SEGMENT_G 0b00000001     // Segment g on PB0
#define DOT 0b00000010           // DP on PB1
#define ANODE_5_BIT 0b00000001   // Anode for digit 5 on PC0

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
  size_t length = 0;

  // First, calculate the length of the input string
  while (charSetData[length] != '\0') {
    length++;
  }

  // Iterate in reverse order
  for (size_t i = length; i > 0; i--) {                         // Start from length and go to 1
    displayCharacter(length - i + 1, charSetData[i - 1], dot);  // Access character in reverse order
    delay(1);
  }
  clearAll();
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

  const char* charSetData = "88888";
  loopDisplay(charSetData);
  Serial.begin(115200);
  Serial.println(F("setup(): begin"));


  clearAll();
  Serial.println(F("setup(): ready"));
}

void loop() {
  const char* charSetData = "88888";
  loopDisplay(charSetData);
}