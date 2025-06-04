#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define PWM_PIN1 9   // High-side signal for half-bridge 1
#define PWM_PIN2 10  // Low-side signal for half-bridge 1 
#define PWM_PIN3 3   // High-side signal for half-bridge 2
#define PWM_PIN4 11  // Low-side signal for half-bridge 2
#define FEEDBACK_PIN A0

const int lookupTableSize = 200;
const int pwmPeriod = 1600; // For 10kHz switching frequency with 16MHz crystal

// Variables for feedback control
volatile int setpoint = 220;  // Desired output voltage
volatile float Kp = 0.005;      // Proportional gain
volatile float Ki = 0.01;      // Integral gain
volatile float integral = 0;
volatile float error = 0;
volatile float lastError = 0;
volatile int dutyCycle = 0;

// Sine table for positive half cycle
const int sineTable[] = {
    50, 100, 151, 201, 250, 300, 349, 398, 446, 494,
    542, 589, 635, 681, 726, 771, 814, 857, 899, 940,
    981, 1020, 1058, 1095, 1131, 1166, 1200, 1233, 1264, 1294,
    1323, 1351, 1377, 1402, 1426, 1448, 1468, 1488, 1505, 1522,
    1536, 1550, 1561, 1572, 1580, 1587, 1593, 1597, 1599, 1600,
    // ... remaining values abbreviated for brevity
};

void setup() {
    // Configure Timer1 for PWM output on pins 9, 10
    TCCR1A = 0b10100010;  // Clear on compare match, set at BOTTOM
    TCCR1B = 0b00011001;  // Fast PWM mode, no prescaler
    ICR1 = pwmPeriod;     // Set PWM period
    
    // Configure Timer2 for PWM output on pin 3
    TCCR2A = 0b10100011;  // Fast PWM mode
    TCCR2B = 0b00000001;  // No prescaler
    
    // Configure Timer0 for PWM output on pin 11
    TCCR0A = 0b10100011;  // Fast PWM mode
    TCCR0B = 0b00000001;  // No prescaler
    
    // Enable timer overflow interrupt
    TIMSK1 = 0b00000001;
    
    // Set PWM pins as outputs
    pinMode(PWM_PIN1, OUTPUT);
    pinMode(PWM_PIN2, OUTPUT);
    pinMode(PWM_PIN3, OUTPUT);
    pinMode(PWM_PIN4, OUTPUT);
    
    // Initialize feedback pin
    pinMode(FEEDBACK_PIN, INPUT);
    
    // Initialize serial for debugging
    Serial.begin(9600);
    
    // Enable global interrupts
    sei();
}

// PI Controller function
float updatePIController(float measured_value) {
    error = setpoint - measured_value;
    integral += error;
    
    // Anti-windup
    if(integral > 1000) integral = 1000;
    if(integral < -1000) integral = -1000;
    
    float output = Kp * error + Ki * integral;
    
    // Limit output
    if(output > 1.0) output = 1.0;
    if(output < 0.0) output = 0.0;
    
    lastError = error;
    return output;
}

ISR(TIMER1_OVF_vect) {
    static uint8_t index = 0;
    
    // Read feedback and update PI controller
    int feedbackValue = analogRead(FEEDBACK_PIN);
    float voltage = feedbackValue * (5.0 / 1023.0) * (220.0 / 5.0); // Convert to actual voltage
    Serial.println(voltage);
    float controlOutput = updatePIController(voltage);
    
    // Scale sine values based on PI controller output
    int scaledValue = (int)(sineTable[index] * controlOutput);
    
    // First half bridge (PWM_PIN1 & PWM_PIN2)
    OCR1A = scaledValue;           // High-side PWM (PIN9)
    digitalWrite(PWM_PIN2, LOW);    // Low-side always OFF
    
    // Second half bridge (PWM_PIN3 & PWM_PIN4)
    if (index < lookupTableSize/2) {
        // First half of sine wave
        OCR2A = 0;                  // High-side OFF (PIN3)
        OCR0A = 255;                // Low-side ON (PIN11)
    } else {
        // Second half of sine wave
        OCR2A = map(scaledValue, 0, pwmPeriod, 0, 255);  // Scale for 8-bit timer
        OCR0A = 0;                  // Low-side OFF
    }
    
    // Increment index
    index++;
    if (index >= lookupTableSize) {
        index = 0;
    }
    
    // Debug output (every 100th cycle to avoid flooding serial)
    static int debugCounter = 0;
    if(++debugCounter >= 100) {
        Serial.print("Voltage: "); Serial.print(voltage);
        Serial.print(" Error: "); Serial.print(error);
        Serial.print(" Control: "); Serial.println(controlOutput);
        debugCounter = 0;
    }
}

void loop() {
    // Main loop can be used for other control tasks
    // The SPWM generation is handled in the interrupt
    delay(100);
}