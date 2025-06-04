#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>

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
static char theTCCR1A = 0b10000010;
static unsigned long int phaseinc, switchFreq;
static double phaseincMult;

// Control variables
static float integral = 0;
static float error = 0;
static float lastError = 0;
static float targetAmplitude = 512;  // Can be modified via serial
static float baseOutput = 50;  

// Function declarations
void processSerialCommand(void);
bool parseCommand(char* cmd, char* type, float* value);
void sendStatus(void);


///////////////////////////
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
///////////////////////////


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
  Serial.println(amp);
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

// Rest of the original functions remain unchanged
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


void setup() {
  Serial.begin(115200);  // Higher baud rate for better responsiveness
  makeLookUp();
  setSwitchFreq(10000);  
  setFreq(50);
  setAmp(baseOutput);
  registerInit();
  
  Serial.println("SPWM Controller Ready");
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