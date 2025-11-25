uint8_t TRANSPOSE = 0;

// * = Adjustable variable
// Keep track the matrix
volatile uint8_t columnPin = 1;   //  Keep this value to 1
volatile uint8_t column = 0;      // *Adjust this value to offset the column
volatile bool    buttonState;     //  To keep track row button state
const int        columnCount = 8; // *How many column

// Button arrays for press and release detection. Also used to list holded button
int currentButton[8][8];
int previousButton[8][8];
int sustainedButton[8][8];

// Sustain pedal release detection
volatile bool currentSustain;
volatile bool previousSustain = false;

// Midi note value mapping
const int midiNote[8][8] = {
  {92, 93, 94, 95, 96, 97, 98, 99},
  {36, 37, 38, 39, 40, 41, 42, 43},
  {44, 45, 46, 47, 48, 49, 50, 51},
  {52, 53, 54, 55, 56, 57, 58, 59},
  {60, 61, 62, 63, 64, 65, 66, 67},
  {68, 69, 70, 71, 72, 73, 74, 75},
  {76, 77, 78, 79, 80, 81, 82, 83},
  {84, 85, 86, 87, 88, 89, 90, 91}
};

////////////////////////////////////
void setup() {
  ADCSRA = 0;           // Disable any ADC
  //TCCR2A = 0;           // Timer2 normal mode
  //TCCR2B = 0x7;         // Timer2 prescaler
  //TIMSK2 = 1;           // Enable timer2 overflow flag
  
  DDRC = 0b00111111;    // Set     PC0-PC5 as output
  DDRD = 0b00001100;    // Set     PD2-PD3 as output and PD4-PD7 as input
  DDRB = 0;             // Set     PORTB   as input
  
  PORTD = 0xf0;         // Pull    PD4-PD7 as HIGH (input)
  PORTB = 0xff;         // Pull    PB0-PB3 as HIGH (input)

  Serial.begin(115200); // Initiate Serial connection
  sei();                // Inititate global interrupt
}

// Used for debugging
void printBinary16(uint16_t value) {
  for (int i = 7; i >= 0; i--) {
    Serial.print((value >> i) & 0x01);
  }
  Serial.println();
}

void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) {
  Serial.write(0x90 | (channel & 0x0f));
  Serial.write((note + TRANSPOSE) & 0x7f);
  Serial.write(velocity & 0x7f);
}

void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) {
  Serial.write(0x80 | (channel & 0x0f));
  Serial.write((note + TRANSPOSE) & 0x7f);
  Serial.write(velocity & 0x7f);
}

void loop() {
  // Update row scanning index
  columnPin = (columnPin << 1) & 0xff; // Circular shift column
  if (columnPin == 0) columnPin = 1;   // Reset columnPin if it overflows
  column = (column + 1) % columnCount; // Update row index
  
  int a = columnPin      & 0b00111111;
  int b = (columnPin>>4) & 0b00001100;
  
  currentSustain = !(PINB & (1 << 4)); // Keep track sustain pedal state

  ////////////////////////////////
  // Send OFF message of all released note when sustain pedal released
  // Send only once on release moment
  if ((currentSustain == 0) && (previousSustain == 1)) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
          if(sustainedButton[i][j]) {
            sendNoteOff(midiNote[i][j], 0);
            sustainedButton[i][j] = 0;
          }
        }
      }
  }
  previousSustain = currentSustain;


  /////////////////////////////
  // Rows Scanning
  for (int i = 0; i < 8; i++) {
    if(i <= 3) {
      buttonState = (~PIND >> (4 + i)) & 1;
      currentButton[column][i] = buttonState;

      if ((currentButton[column][i]) && !(previousButton[column][i])) {
        //Serial.print(midiNote[column][i]); Serial.println(" ON");
        sendNoteOn(midiNote[column][i],127);
        if(currentSustain) {
          sustainedButton[column][i] = 1;
        }
      }
    
      if (!(currentButton[column][i]) && (previousButton[column][i]) && (!currentSustain)) {
        //Serial.print(midiNote[column][i]); Serial.println(" OFF");
        sendNoteOff(midiNote[column][i],127);
      }
      previousButton[column][i] = currentButton[column][i];
    }
    else {
      buttonState = (~PINB >> (i-4)) & 1;
      currentButton[column][i] = buttonState;

      if ((currentButton[column][i]) && !(previousButton[column][i])) {
        //Serial.print(midiNote[column][i]); Serial.println(" ON");
        sendNoteOn(midiNote[column][i],127);
        if(currentSustain) {
          sustainedButton[column][i] = 1;
        }
      }
    
      if (!(currentButton[column][i]) && (previousButton[column][i]) && (!currentSustain)) {
        //Serial.print(midiNote[column][i]); Serial.println(" OFF");
        sendNoteOff(midiNote[column][i],127);
      }
      previousButton[column][i] = currentButton[column][i];
    }
  }
  
  PORTD = ~b;              // Output inverted columnPin value to PORTD
  PORTC = a ^ 0b00111111;  // Output inverted columnPin value to PORTC
}
