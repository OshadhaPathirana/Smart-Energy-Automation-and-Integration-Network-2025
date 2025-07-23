#include <SPI.h>
#include <LoRa.h>

#define SS 4      // GPIO4 (D2)
#define RST 5     // GPIO5 (D1)
#define DIO0 16   // GPIO16 (D0)

void setup() {
  Serial.begin(9600);
  while (!Serial);

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check wiring.");
    while (true);
  }

  Serial.println("LoRa init succeeded.");
}

void loop() {
  String message = "Voltage: 230.0, Frequency: 50.0";
  Serial.println("Sending: " + message);

  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();

  delay(5000); // wait 5 seconds
}
