#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define LookupEntries (512)
#define TARGET_AMPLITUDE 512    // Target amplitude (50% of max)
#define KP 0.5                 // Proportional gain
#define KI 0.1                 // Integral gain
#define SAMPLING_TIME 50       // Sampling time in milliseconds
#define PRINT_INTERVAL 250     // Print interval in milliseconds

static int microMHz = 16;      // clock frequency in MHz
static int freq, amp = 1024;   // Sinusoidal frequency
static long int period;        // Period of PWM in clock cycles
static unsigned int lookUp[LookupEntries];
static char theTCCR1A = 0b10000010;
static unsigned long int phaseinc, switchFreq;
static double phaseincMult;

// Variables for PI control
static float integral = 0;
static float error = 0;
static float lastError = 0;
static float targetAmplitude = TARGET_AMPLITUDE;
static float baseOutput = 50;  // Base output level (50% of max)

// Variables for testing
static unsigned long lastPrintTime = 0;
static int testPhase = 0;
static float potValue = 0;

// Function declarations
int setFreq(int freq);         
int setSwitchFreq(int sfreq);  
int setAmp(float _amp);        
void makeLookUp(void);
void registerInit(void);
float calculatePI(float measured);
void printSystemStatus(float measured, float controlOutput);

void setup() {
  Serial.begin(9600);
  makeLookUp();
  setSwitchFreq(10000);  
  setFreq(50);
  setAmp(baseOutput);  // Start at base output level
  registerInit();
  
  Serial.println("Testing PWM Amplitude Control System");
  Serial.println("Turn potentiometer to simulate load changes");
  Serial.println("Format: Time(ms), Target, Measured, Control Output, Error");
}

void loop() {
  static unsigned long lastTime = 0;
  static int measuredAmp;
  unsigned long currentTime = millis();
  
  // Execute control loop every SAMPLING_TIME milliseconds
  if (currentTime - lastTime >= SAMPLING_TIME) {
    lastTime = currentTime;
    
    // Read potentiometer value (0-1023)
    potValue = analogRead(A1);
    
    // Simulate a disturbance by using pot value to affect measured amplitude
    measuredAmp = potValue;
    
    // Calculate and apply new amplitude using PI control
    float controlOutput = calculatePI(measuredAmp);
    setAmp(controlOutput);
    
    // Print system status at regular intervals
    if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
      printSystemStatus(measuredAmp, controlOutput);
      lastPrintTime = currentTime;
    }
  }
}

void printSystemStatus(float measured, float controlOutput) {
  Serial.print(millis());
  Serial.print(",");
  Serial.print(targetAmplitude);
  Serial.print(",");
  Serial.print(measured);
  Serial.print(",");
  Serial.print(controlOutput);
  Serial.print(",");
  Serial.println(error);
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

ISR(TIMER1_OVF_vect) {
  static unsigned long int phase, lastphase;
  static char delay1, trig = LOW;
  phase += phaseinc;
  if(delay1 == 1) {
    theTCCR1A ^= 0b10100000;
    TCCR1A = theTCCR1A;
    delay1 = 0;  
  }
  else if((phase>>31 != lastphase>>31) && !(phase>>31)) {
    delay1++;      
    trig = !trig;
    digitalWrite(13,trig);
  }
 
  lastphase = phase;
  OCR1A = OCR1B = ((lookUp[phase >> 23]*period) >> 12)*amp >> 10;
}

// Original helper functions remain unchanged
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
    amp = map(_amp,0,100,0,1024);
    return 1;
  }  
}

void makeLookUp(void) {
  double temp;
  cli();
  TCCR1A = 0b00000010;
   
  for(int i = 0; i < LookupEntries; i++) {
    temp = sin(i*M_PI/LookupEntries)*4096;
    lookUp[i] = (int)(temp+0.5);
  }
  TCCR1A = theTCCR1A;
  sei();
}

void registerInit(void) {
  TCCR1A = theTCCR1A;
  TCCR1B = 0b00011001;
  TIMSK1 = 0b00000001;
  sei();
  DDRB = 0b00000110;
  pinMode(13, OUTPUT);
}