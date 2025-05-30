#include <WiFi.h>
#include <MycilaJSY.h>

#define RX2 16
#define TX2 17

const char *ssid = "";  // Your WiFi SSID
const char *password = "";  // Your WiFi password

WiFiServer server(80);
Mycila::JSY jsy;

void setup() {
  Serial.begin(115200);
  jsy.begin(Serial2, RX2, TX2);

  // Connect to WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();  // Check if a client is connected

  if (client) {  
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        
        if (request.endsWith("\r\n\r\n")) {  // End of HTTP request
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();

          // HTML Page Header
          client.println("<!DOCTYPE html>");
          client.println("<html><head><title>Power Monitor</title>");
          client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
          client.println("<meta http-equiv='refresh' content='1'>"); // Refresh every second
          client.println("<style>");
          client.println("body { font-family: Arial, sans-serif; text-align: center; background-color: #f4f4f4; padding: 20px; }");
          client.println(".container { max-width: 400px; margin: auto; background: white; padding: 20px; box-shadow: 0px 0px 10px 0px #aaa; }");
          client.println("h2 { color: #333; }");
          client.println(".data { font-size: 20px; margin: 10px 0; }");
          client.println("</style>");
          client.println("</head><body>");
          client.println("<div class='container'>");
          client.println("<h2>JSY-MK-194G Power Monitor</h2>");

          // Read from sensor
          if (jsy.read()) {
            float v1 = jsy.data.channel1().voltage;
            float i1 = jsy.data.channel1().current;
            float f1 = jsy.data.channel1().frequency;
            float rp1 = jsy.data.channel1().reactivePower;
            float ap1 = jsy.data.channel1().apparentPower;
            float pf1 = jsy.data.channel1().powerFactor;

            // Write to webpage
            client.printf("<p class='data'><b>Voltage:</b> %.2f V</p>", v1);
            client.printf("<p class='data'><b>Current:</b> %.2f A</p>", i1);
            client.printf("<p class='data'><b>Frequency:</b> %.2f Hz</p>", f1);
            client.printf("<p class='data'><b>Reactive Power:</b> %.2f VAR</p>", rp1);
            client.printf("<p class='data'><b>Apparent Power:</b> %.2f VA</p>", ap1);
            client.printf("<p class='data'><b>Power Factor:</b> %.2f</p>", pf1);

            // Write to Serial Monitor
            Serial.println("---- Power Monitor Data ----");
            Serial.printf("Voltage: %.2f V\n", v1);
            Serial.printf("Current: %.2f A\n", i1);
            Serial.printf("Frequency: %.2f Hz\n", f1);
            Serial.printf("Reactive Power: %.2f VAR\n", rp1);
            Serial.printf("Apparent Power: %.2f VA\n", ap1);
            Serial.printf("Power Factor: %.2f\n", pf1);
            Serial.println("----------------------------");
            delay(200);

          } else {
            client.println("<p class='data' style='color:red;'><b>Error:</b> Failed to read from JSY-MK-194G!</p>");
            Serial.println("Error: Failed to read from JSY-MK-194G!");
          }

          client.println("</div>");
          client.println("</body></html>");
          client.println();
          
          break; // Exit the request processing loop
        }
      }
    }

    while (client.available()) { client.read(); } // Clear remaining data
    client.stop(); // Close connection
  }

  delay(50); // Prevent overload
}
