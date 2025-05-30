#include <MycilaJSY.h>
#define RX2 16
#define TX2 17

bool isOn = false;
const char *ssid = ""; // Your WiFi SSID
const char *password = ""; // Your WiFi Password

Mycila::JSY jsy;

void setup() {
  jsy.begin(Serial2, RX2, TX2);
  Serial.begin(9600);
}

void loop() {

   WiFiClient client = server.available();  // Listen for incoming clients

  if (client) {  // If a client connects
    Serial.println("New Client.");
    String currentLine = "";  // To hold incoming client data

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send HTTP response
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // HTML for controlling the LEDs
            client.print("Click <a href=\"/ON\">here</a> to turn the LEDs ON.<br>");
            client.print("Click <a href=\"/OFF\">here</a> to turn the LEDs OFF.<br>");

            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }

        // Check if the client request is to turn LEDs ON or OFF
        if (currentLine.endsWith("GET /ON")) {
          isOn = true;  // Turn LEDs ON
        }
        if (currentLine.endsWith("GET /OFF")) {
          isOn = false;  // Turn LEDs OFF
        }
      }
    }
    client.stop();
    Serial.println("Client Disconnected.");
  }

  if (jsy.read()) {
   

    float v1 = jsy.data.channel1().voltage; // for JSY-193 and JSY-19 
    Serial.print("Voltage ");
    Serial.print(v1);

    float i1 = jsy.data.channel1().current;
    Serial.print("  Current ");
    Serial.print(i1);

    float f1 = jsy.data.channel1().frequency;
    Serial.print("  Frequency ");
    Serial.print(f1);

    float rp1 = jsy.data.channel1().reactivePower;
    Serial.print("  Reactive Power ");
    Serial.print(rp1);

    float ap1 = jsy.data.channel1().apparentPower;
    Serial.print("  Apparent Power ");
    Serial.print(rp1);
 
    float pf1 = jsy.data.channel1().powerFactor;
    Serial.print("  Power factor  ");
    Serial.println(pf1);

   
  }
  delay(4000);
}