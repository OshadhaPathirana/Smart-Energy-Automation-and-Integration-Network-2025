#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>

#define LookupEntries (512)
#define KP 0.5                 
#define KI 0.1                 
#define SAMPLING_TIME 50       
#define PRINT_INTERVAL 250     

// Serial command buffer
#define MAX_COMMAND_LENGTH 32
char cmdBuffer[MAX_COMMAND_LENGTH];
int cmdIndex = 0;

static int microMHz = 16;      
static int freq = 50, amp = 1024;   
static long int period;        
static unsigned int lookUp[LookupEntries];
static unsigned long int phaseinc, switchFreq;
static double phaseincMult;

// Control variables
static float integral = 0;
static float error = 0;
static float lastError = 0;
static float targetAmplitude = 512;  // Can be modified via serial
static float baseOutput = 50;  

// Phase tracking for switching
static unsigned long int phase = 0;
static unsigned long int lastPhase = 0;
static boolean outputToPin11 = true; // true = output on Pin 11, false = output on Pin 10

// Function declarations
void processSerialCommand(void);
bool parseCommand(char* cmd, char* type, float* value);
void sendStatus(void);

void setup() {
  Serial.begin(115200);  
  
  // Configure Timer1 for pin 10 (OC1B)
  // Fast PWM mode, TOP = ICR1
  TCCR1A = 0;
  TCCR1B = 0;
  
  // Configure Timer2 for pin 11 (OC2A)
  TCCR2A = 0;
  TCCR2B = 0;
  
  // Make lookup table first (with interrupts disabled)
  cli();  // Disable interrupts
  makeLookUp();
  setSwitchFreq(10000);  
  setFreq(50);
  setAmp(baseOutput);
  
  // Set pins 10 and 11 as outputs
  pinMode(10, OUTPUT); // OC1B
  pinMode(11, OUTPUT); // OC2A
  pinMode(13, OUTPUT); // Debug LED
  
  // Initialize Timer1 for Pin 10 (OC1B)
  // Set Timer1 to Fast PWM mode, TOP = ICR1
  ICR1 = period;
  TCCR1A = (0 << COM1A1) | (0 << COM1A0) | (0 << COM1B1) | (0 << COM1B0) | (1 << WGM11) | (0 << WGM10);
  TCCR1B = (1 << WGM13) | (0 << WGM12) | (0 << CS12) | (0 << CS11) | (1 << CS10); // No prescaler
  
  // Initialize Timer2 for Pin 11 (OC2A)
  // Fast PWM mode
  TCCR2A = (0 << COM2A1) | (0 << COM2A0) | (0 << COM2B1) | (0 << COM2B0) | (1 << WGM21) | (1 << WGM20);
  TCCR2B = (0 << WGM22) | (0 << CS22) | (0 << CS21) | (1 << CS20); // No prescaler
  
  // Enable Timer1 overflow interrupt
  TIMSK1 = (1 << TOIE1);
  
  sei();  // Enable interrupts
  
  // Start with Pin 11 active
  TCCR2A |= (1 << COM2A1);
  
  Serial.println("SPWM Controller Ready - Using Pins 10 and 11");
  Serial.println("Commands:");
  Serial.println("A<value> - Set target amplitude (0-1023)");
  Serial.println("F<value> - Set frequency (1-1000 Hz)");
  Serial.println("S - Request status");
}

void loop() {
  static unsigned long lastTime = 0;
  static int measuredAmp;
  unsigned long currentTime = millis();
  
  // Process any incoming serial commands
  if (Serial.available()) {
    processSerialCommand();
  }
  
  // Regular control loop
  if (currentTime - lastTime >= SAMPLING_TIME) {
    lastTime = currentTime;
    
    // Read current amplitude (you may need to modify this based on your hardware)
    measuredAmp = analogRead(A1);
    
    // Calculate and apply new amplitude using PI control
    float controlOutput = calculatePI(measuredAmp);
    setAmp(controlOutput);
  }
}

void processSerialCommand() {
  while (Serial.available() > 0) {
    char inChar = Serial.read();
    
    if (inChar == '\n' || inChar == '\r') {
      if (cmdIndex > 0) {
        cmdBuffer[cmdIndex] = '\0';  // Null terminate
        char cmdType;
        float value;
        
        // Parse and execute command
        if (parseCommand(cmdBuffer, &cmdType, &value)) {
          switch (cmdType) {
            case 'A':  // Set amplitude
              if (value >= 0 && value <= 1023) {
                targetAmplitude = value;
                Serial.print("New target amplitude: ");
                Serial.println(targetAmplitude);
              }
              break;
              
            case 'F':  // Set frequency
              if (setFreq((int)value)) {
                Serial.print("New frequency: ");
                Serial.println(freq);
              }
              break;
              
            case 'S':  // Status request
              sendStatus();
              break;
          }
        }
        cmdIndex = 0;  // Reset buffer
      }
    }
    else if (cmdIndex < MAX_COMMAND_LENGTH - 1) {
      cmdBuffer[cmdIndex++] = inChar;
    }
  }
}

bool parseCommand(char* cmd, char* type, float* value) {
  *type = cmd[0];  // First character is command type
  
  if (*type == 'S') {
    return true;  // Status command doesn't need a value
  }
  
  // Convert rest of string to float
  *value = atof(cmd + 1);
  return true;
}

void sendStatus() {
  Serial.print("Status: F=");
  Serial.print(freq);
  Serial.print("Hz, A=");
  Serial.print(targetAmplitude);
  Serial.print(", Measured=");
  Serial.print(analogRead(A1));
  Serial.print(", Control=");
  Serial.print(amp);
  Serial.print(", Active Pin: ");
  Serial.println(outputToPin11 ? "Pin 11" : "Pin 10");
}

float calculatePI(float measured) {
  error = targetAmplitude - measured;
  
  // Calculate integral term with anti-windup
  integral = constrain(integral + (error * SAMPLING_TIME / 1000.0), -50, 50);
  
  // Calculate PI control output centered around baseOutput
  float output = baseOutput + (KP * error) + (KI * integral);
  
  // Constrain output to valid range
  output = constrain(output, 0, 100);
  
  // Store error for next iteration
  lastError = error;
  
  return output;
}

// Timer1 overflow interrupt
ISR(TIMER1_OVF_vect) {
  static bool toggleReady = false;
  
  // Update phase accumulator
  phase += phaseinc;
  
  // Calculate PWM value from lookup table
  unsigned int pwmValue = ((lookUp[(phase >> 23) & 0x1FF] * period) >> 12) * amp >> 10;
  
  // For Timer2, we need to scale the pwmValue to fit in 8-bit range (0-255)
  uint8_t pwmValue8bit = map(pwmValue, 0, period, 0, 255);
  
  // Check for half-cycle (zero crossing)
  // We detect this by checking when the highest bit changes from 1 to 0
  if ((phase >> 31) == 0 && (lastPhase >> 31) == 1) {
    toggleReady = true;
    digitalWrite(13, !digitalRead(13)); // Toggle debug LED
  }
  
  // If we've detected a zero crossing, toggle which pin is active
  if (toggleReady) {
    outputToPin11 = !outputToPin11;
    toggleReady = false;
    
    // Reconfigure timer outputs
    if (outputToPin11) {
      // Pin 11 (OC2A) active, Pin 10 (OC1B) forced low
      TCCR2A = (1 << COM2A1) | (0 << COM2A0) | (0 << COM2B1) | (0 << COM2B0) | (1 << WGM21) | (1 << WGM20);
      TCCR1A = (0 << COM1A1) | (0 << COM1A0) | (0 << COM1B1) | (0 << COM1B0) | (1 << WGM11) | (0 << WGM10);
      OCR1B = 0; // Force pin 10 low
    } else {
      // Pin 10 (OC1B) active, Pin 11 (OC2A) forced low
      TCCR1A = (0 << COM1A1) | (0 << COM1A0) | (1 << COM1B1) | (0 << COM1B0) | (1 << WGM11) | (0 << WGM10);
      TCCR2A = (0 << COM2A1) | (0 << COM2A0) | (0 << COM2B1) | (0 << COM2B0) | (1 << WGM21) | (1 << WGM20);
      OCR2A = 0; // Force pin 11 low
    }
  }
  
  // Set the PWM value for the active pin
  if (outputToPin11) {
    OCR2A = pwmValue8bit;
  } else {
    OCR1B = pwmValue;
  }
  
  lastPhase = phase;
}

int setFreq(int _freq) {
  if(_freq < 0 || _freq > 1000) {
    return 0;
  } else {
    freq = _freq;
    phaseinc = (unsigned long int) phaseincMult*_freq;
    return 1;
  }
}
   
int setSwitchFreq(int sfreq) {
  if(sfreq <= 0 || sfreq > 20000) {
    return 0;
  } else {
    switchFreq = sfreq;
    period = microMHz*1e6/sfreq;
    phaseincMult = (double) period*8589.934592/microMHz;
    phaseinc = (unsigned long int) phaseincMult*freq;
    ICR1 = period;
    return 1;
  }
}

int setAmp(float _amp) {
  if(_amp < 0 || _amp > 100) {
    return 0;
  } else {
    amp = map(_amp, 0, 100, 0, 1024);
    return 1;
  }  
}

void makeLookUp(void) {
  double temp;
  
  for(int i = 0; i < LookupEntries; i++) {
    temp = sin(i*M_PI/LookupEntries)*4096;
    lookUp[i] = (int)(temp+0.5);
  }
}