
/*-----------------------------------------------------------------------* 
 *                                                                       *
 *   Program:      Euclidean Rhythm Generator Kosmo/Eurorack             *
 *   Author:       L J Brackney,                                         *
 *   Organization: Suncoast Polytechnical High School                    *
 *   Version:      1.0                                                   *
 *   Date:         11/21/2021                                            *
 *                                                                       *
 *-----------------------------------------------------------------------*/

#include <Adafruit_NeoPixel.h>    // Include Adafruit_NeoPixel library
#include <Encoder.h>              // Include rotary encoder library

Encoder myEnc(3, 4);              // Attach rotary encoder to digital pins 3 and 4

#define PixelPin    2             // Attach NeoPixel ring to digital pin 2
#define ProgPin     5             // Attach rotary encoder switch to digital pin 5
#define ClkPin      6             // Attach external clock select switch to digital pin 6
#define Trig1Pin   10             // Attach the four drum/voice trigger output pins to 
#define Trig2Pin    8             //   digital pins 7-10.  I mixed up two wires when
#define Trig3Pin    9             //   assembling the modules, and it was easier to
#define Trig4Pin    7             //   swap pins 7 and 10 than to resolder things. :)
#define TapPin     11             // Attach a push button for the "tap tempo" feature to digital pin 11
#define ButtonPin  A0             // The trigger select buttons are on a voltage divider attached to A0
#define PotPin     A1             // The pulse width potentiometer is attached to analog pin A1
#define ExtClkPin  A2             // The external clock CV is attached to analog pin A2

#define NumSteps   16             // The program assumes 16 beat sequences, but it can be changed here

int  Part = 1;                    // The current part/trigger is initially set to 1.

Adafruit_NeoPixel pixels(NumSteps, PixelPin, NEO_GRB + NEO_KHZ800);  // Set up the NeoPixel ring


// The Euclid function returns an unsigned integer whose bits reflect a Euclidean rhythm of
// "Numpulses" spread across "NumSteps."  There are several ways to implement this, but I
// adapted the algorithm described at:
//   https://www.computermusicdesign.com/simplest-euclidean-rhythm-algorithm-explained/

unsigned int Euclid(int NumPulses) {
  unsigned int Number = 0;              // Number is the number of pulses that have been allocated so far
  for (int i=0; i<NumSteps; i++) {      // Cycle through all of the possible steps in the sequence
    int bucket = bucket + NumPulses;    // Fill a "bucket" with the number of pulses to be allocated
    if (bucket >= NumSteps) {           // If the bucket exceeds the maximum number of steps then "empty
      bucket = bucket - NumSteps;       // the bucket" by setting the "ith" bit in the sequence and
      Number |= 1 << i;                 // refill the now empty bucket with the remainder.
    }
  }
  return(Number);                       // Return the sequence encoded as the bits set in Number
}


// The RotateLeft function shifts the bits in an unsigned integer by one place.  Any bit that falls
// off to the left (of the most significant bit) is shifted around to the become the new least
// significant bit.  For our purposes, this means that we can rotate the Euclidean Rhythm clockwise
// on the ring. 

unsigned int RotateLeft(unsigned int Number) {
  int DROPPED_MSB;                              // Need to keep track of any bits dropped off the left
  DROPPED_MSB = (Number >> NumSteps-1) & 1;     // aka the old most significant bit
  Number = (Number << 1) | DROPPED_MSB;         // Shift all the bits in Number to the left by 1 and
  return(Number);                               // add back in the new least significant bit if needed
}


// The RotateRight function shifts the bits in an unsigned integer by one place.  Any bit that falls
// off to the right (of the least significant bit) is shifted around to the become the new most
// significant bit.  For our purposes, this means that we can rotate the Euclidean Rhythm counter
// clockwise on the ring.

unsigned int RotateRight(unsigned int Number) {
  int DROPPED_LSB;                               // Need to keep track of any bits dropped off the right
  DROPPED_LSB = Number & 1;                      // aka the old least signficant bit
  Number = (Number >> 1) & (~(1 << NumSteps-1)); // Shift all the bits in Number to the right by 1
  Number = Number | (DROPPED_LSB << NumSteps-1); // Tack the old LSB onto the new MSB if needed
  return(Number);
}


// CheckButtons decodes the voltage values from the button voltage divider circuit.  The values here
// assume that 1k, 2.2k, 4.7k and 10k resistors are switched in line with a 5V source and dropped
// across a 10k resistor before going into the analog input channel.  The ranges should be big
// enough to accomodate resistor variation, but if a button doesn't switch to the trigger as expected
// you should Serial.println(Buttons) and adjust the ranges as needed for your circuit.

int CheckButtons() {
  int Buttons = analogRead(ButtonPin);         // Read the voltage divider output
  if ((Buttons>480)&&(Buttons<520)) Part = 1;  // Select pattern/trigger 1 if the first button is pressed
  if ((Buttons>580)&&(Buttons<620)) Part = 2;  // Select pattern/trigger 2 if the second button is pressed
  if ((Buttons>680)&&(Buttons<720)) Part = 3;  // Select pattern/trigger 3 if the third button is pressed
  if (Buttons>800) Part = 4;                   // Select pattern/trigger 4 if the last button is pressed
}


// ClearPattern erases all lights on the NeoPixel in preparation for an updated pattern to be displayed

void ClearPattern() {
  for (int i=0; i<NumSteps; i++)                   // Step through each pixel in the ring
    pixels.setPixelColor(i, pixels.Color(0,0,0));  // turn each pixel off
  pixels.show();                                   // Update the pixels displayed on the ring
}


// BitPattern displays a Euclidean pattern associated with the bits set in Number.  The color of the
// pattern is dictated by channel number "Chan."  You can change the ring colors to match the buttons
// and output LEDs you use in your build here.

void BitPattern(int Chan, unsigned int Number) {
  int R;
  int G;
  int B;
  switch (Chan) {
    case 1: R = 20; G = 0;  B = 0;  break;  // Pattern/trigger 1 will be shown in red
    case 2: R = 0;  G = 20; B = 0;  break;  // Pattern/trigger 2 will be shown in green
    case 3: R = 0;  G = 0;  B = 20; break;  // Pattern/trigger 3 will be shown in blue
    case 4: R = 20; G = 20; B = 0;  break;  // Pattern/trigger 4 will be shown in yellow 
  }
  for (int i=0; i<NumSteps; i++)            // Step through each pixel in the ring
    if (Number & (1<<i))                    // If the ith bit is set, then set that pixel to the appropriate color
       pixels.setPixelColor(i, pixels.Color(R,G,B));
    else                                    // Otherwise turn the pixel off
       pixels.setPixelColor(i, pixels.Color(0,0,0));       
  pixels.show();                            // Update the pixels displayed on the ring
}


void setup() {
  Serial.begin(9600);                       // Set up the serial console for debugging messages
  pinMode(ProgPin,INPUT_PULLUP);            // Assign the encoder switch which closes to ground and uses an internal pullup resistor
  pinMode(ClkPin,INPUT);                    // Assign the external clock selector switch
  pinMode(Trig1Pin,OUTPUT);                 // Assign trigger 1 out
  pinMode(Trig2Pin,OUTPUT);                 // Assign trigger 2 out
  pinMode(Trig3Pin,OUTPUT);                 // Assign trigger 3 out
  pinMode(Trig4Pin,OUTPUT);                 // Assign trigger 4 out
  pinMode(TapPin,INPUT);                    // Assign the button for "Tap Tempo"
  pixels.begin();                           // Start the NeoPixel
  pixels.clear();                           // And clear it
}


void loop() { 
  static unsigned int Ch1 = 0x8888;         // Start up the generator with default bit patterns for triggers 1-4
  static unsigned int Ch2 = 0x4444;         // Each number/pattern persists between loop iterations
  static unsigned int Ch3 = 0x2222;
  static unsigned int Ch4 = 0xEEEE;
  
  static int  Step        = 0;              // The active step persists in each loop but starts out at 0 on boot
  static int  Delay       = 80;             // The time delay between steps persists in each loop but starts out at 80 msec
  static int  Mode        = 0;              // The module mode persists in each loop
  static long Mode1Pos    = 0;              // The last encoder position in mode 1 is stored for a future return to that mode
  static int  PrevExtClk  = 0;              // Store the previous External Clock Pulse value for transition checking
  bool        Triggered   = false;          // Assume that no new step is going to occur this loop
  static unsigned long Time;                // Prepare to query the Nano's clock
  static unsigned long PrevTime = 0;        // Keep track of how much time elapses between loops
  static long oldPosition;                  // Keep track of the previous encoder count
  static int  Pulses;                       // The number of pulses to be generated for the active trigger
  static bool PrevProg;                     // Keep track of the previous encoder switch state
  static bool Prog;                         // Prepare to read the current encoder switch state
  
  long newPosition = myEnc.read();          // Read the current number of encoder counts
  int  PotV        = analogRead(PotPin);    // Read the trigger pulse width potentiometer value
  int  ExtClkV     = analogRead(ExtClkPin); // Read the external clock input
  CheckButtons();                           // Query the trigger buttons and update the active trigger if needed
   
  PrevProg = Prog;                          // Store the last encoder switch position
  Prog     = digitalRead(ProgPin);          // Get the current encoder switch position

  Time = millis();                          // Get the current Nano clock value in msec

  if ((Prog==true) && (PrevProg==false)) {  // If the encoder switch was just pressed, we need to switch modes
     if (Mode==1) Mode1Pos = newPosition;   // Store the current encoder position for a future return to that mode
     Mode++;                                                          // Cycle to the next mode
     if (Mode>3) Mode=1;                                              // Go back to mode 1 after mode 3
     if (Mode==1) { myEnc.write(Mode1Pos); newPosition = Mode1Pos; }  // Restore the encoder position when last in the current mode
     oldPosition = newPosition;                                       // The current encoder position will be the old position next loop
  }  

  ClearPattern();                              // Clear the NeoPixel ring in preparation for any changes to the pattern
     
  switch (Mode) {
    case 1:                                    // Mode 1 - Manually adjust the pattern tempo (based on the internal clock)
      if (newPosition!=oldPosition) {          // Only execute code if the encoder has moved
         Delay = newPosition*2;                // The internal time delay is proportional to the encoder position
         if (Delay<10) Delay = 10;             // Limit the fastest loop time to 10 msec per step, which is crazy fast
         if (Delay>1000) Delay = 1000;         // Limit the slowest loop time to 1 sec per step.  Change if you want slower tempos.
         oldPosition = newPosition;            // The current encoder position will be the old position next loop
      }
      break;   
    case 2:                                    // Mode 2 - Change number of Euclidean Rhythm pulses of the active pattern
      if (newPosition < oldPosition-3) {       // Do something if the encoder has increased by 1 detent (4 pulses for my encoder)
        Pulses++;                              // Increase the number of pulses in the active pattern
        if (Pulses>NumSteps) Pulses=NumSteps;  // Limit the largest number of pulses to NumSteps
        oldPosition = newPosition;             // The current encoder position will be the old position next loop
      }
      else if (newPosition > oldPosition+3) {  // Do something if the encoder has decreased by 1 detent (4 pulses for my encoder)
        Pulses--;                              // Decrease the number of pulses in the active pattern
        if (Pulses<0) Pulses=0;                // Limit the smallest number of pulses to zero
        oldPosition = newPosition;             // The current encoder position will be the old position next loop
      }
      Serial.println(Pulses);
      switch (Part) {                          // Compute a Euclidean Rhythm for the active pattern
        case 1: Ch1 = Euclid(Pulses); break;
        case 2: Ch2 = Euclid(Pulses); break;
        case 3: Ch3 = Euclid(Pulses); break;
        case 4: Ch4 = Euclid(Pulses); break;
      }
      break;
    case 3:                                    // Mode 3 - Rotate the Euclidean Rhythm of the active pattern
      if (newPosition > oldPosition+3) {       // Do something if the encoder has increased by 1 detent (4 pulses for my encoder)
        oldPosition = newPosition;             // The current encoder position will be the old position next loop
        switch (Part) {                        // Rotate the active pattern one bit to the right (counter clockwise)
          case 1: Ch1 = RotateRight(Ch1); break;
          case 2: Ch2 = RotateRight(Ch2); break;
          case 3: Ch3 = RotateRight(Ch3); break;
          case 4: Ch4 = RotateRight(Ch4); break;
        } 
      }  
      else if (newPosition < oldPosition-3) {  // Do something if the encoder has decreased by 1 detent (4 pulses for my encoder)
        oldPosition = newPosition;             // The current encoder position will be the old position next loop
        switch (Part) {                        // Rotate the active pattern one bit to the left (clockwise)
          case 1: Ch1 = RotateLeft(Ch1); break;
          case 2: Ch2 = RotateLeft(Ch2); break;
          case 3: Ch3 = RotateLeft(Ch3); break;
          case 4: Ch4 = RotateLeft(Ch4); break;
        }
      }
      break;
  }    

  switch(Part) {                               // Display the active pattern along with any updates from the current mode
    case 1: BitPattern(1,Ch1); break;
    case 2: BitPattern(2,Ch2); break;
    case 3: BitPattern(3,Ch3); break;
    case 4: BitPattern(4,Ch4); break;
  }

  if (digitalRead(ClkPin)==true) {             // If we're using an external clock pulse
     if ((ExtClkV>512) && (PrevExtClk<128)) {  // Check to see if the pulse just went high
        Delay     = Time-PrevTime;             // If so, then capture the msec delay associated with the external pulse
        Triggered = true;                      // Note that we've just triggered a new step in the sequence
     }
     PrevExtClk = ExtClkV;                     // The current external clock input will be the previous input for the next loop
  }
  else                                         // If we're using the internal clock to generate pulses
  {
    if ((Time-PrevTime)>Delay) {               // Check to see if we've waited long enough for the next step in the sequence
      Triggered = true;                        // Note that we've triggered a new step in the sequence
    }
  }

  int TrigWidth = map(PotV, 0, 1023, Delay/2, Delay/50);  // Calculate the pulse width based on the potentiometer setting and clock

  if ((Time-PrevTime)>TrigWidth) {                        // Turn off the the trigger outputs if the pulse width has elapsed
    digitalWrite(Trig1Pin,LOW);
    digitalWrite(Trig2Pin,LOW);
    digitalWrite(Trig3Pin,LOW);
    digitalWrite(Trig4Pin,LOW);  
  }
  
  if (Triggered) {                              // If we're triggering a new step in the sequence
    PrevTime = Time;                            // The current time will be the previous time in the next loop
    Step++;                                     // Increment the step counter
    if (Step>NumSteps-1) Step = 0;              // Reset the step counter if we've gone through all the beats in the sequence
    if (Mode==1) pixels.setPixelColor(Step, pixels.Color(40,40,40)); // Light up the current step pixel with a color to indicate
    if (Mode==2) pixels.setPixelColor(Step, pixels.Color(40,0,40));  // the current mode.
    if (Mode==3) pixels.setPixelColor(Step, pixels.Color(20,20,0));  // White = Tempo, Violet = Pulses, and Yellow = Rotation
    pixels.show();                                                   // Display the step/mode pixel.
  
    if (Ch1 & (1<<Step)) digitalWrite(Trig1Pin, HIGH);  // Turn on trigger output 1 if the current step is in the Euclidean Rhythm
    if (Ch2 & (1<<Step)) digitalWrite(Trig2Pin, HIGH);  // Turn on trigger output 2 if the current step is in the Euclidean Rhythm
    if (Ch3 & (1<<Step)) digitalWrite(Trig3Pin, HIGH);  // Turn on trigger output 3 if the current step is in the Euclidean Rhythm
    if (Ch4 & (1<<Step)) digitalWrite(Trig4Pin, HIGH);  // Turn on trigger output 4 if the current step is in the Euclidean Rhythm
  }  
}
