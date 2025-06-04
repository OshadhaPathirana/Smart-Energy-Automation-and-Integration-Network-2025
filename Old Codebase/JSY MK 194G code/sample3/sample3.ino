#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <MycilaJSY.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define RX2 16
#define TX2 17

#define WIFI_SSID ""   // Your WiFi SSID
#define WIFI_PASSWORD "" // Your WiFi password
#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "kBLLkVYJr_s0oz0bqfnZDFk8AH-Kp8P56uwqWeZWfgJs7WV1HfHknQpyDMiXJbsqaReU_AOxtdq2MB6_sFol7w=="
#define INFLUXDB_ORG "8016ae0a3fa30f7e"
#define INFLUXDB_BUCKET "Bucket1"

// Time zone info
#define TZ_INFO "UTC5.5"

Mycila::JSY jsy;

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Declare Data point
Point sensor("Inverters");

void setup() {
  Serial.begin(115200);
  jsy.begin(Serial2, RX2, TX2);

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  /*// Add tags to the data point
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());*/
}

void loop() {
   if (jsy.read()) {
      float v_1 = jsy.data.channel1().voltage;
      float i_1 = jsy.data.channel1().current;
      float f_1 = jsy.data.channel1().frequency;

      int v1 = v_1;
      int f1 = f_1;
      int i1 = i_1;

      // Clear fields for reusing the point. Tags will remain the same as set above.
      sensor.clearFields();

      // Store measured value into point
      sensor.addField("Voltage", v1);
      sensor.addField("Current", i1);
      sensor.addField("Frequency", f1);

      // Print what are we exactly writing to InfluxDB
      Serial.print("Writing to InfluxDB: ");
      Serial.println(sensor.toLineProtocol());

      // Print the same result to Serial Monitor
      Serial.print("Voltage: ");
      Serial.println(v1);
      Serial.print("Current: ");
      Serial.println(i1);
      Serial.print("Frequency: ");
      Serial.println(f1);
   }

  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi connection lost");
  }

  // Write point to InfluxDB
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  Serial.println("Waiting 1 second");
  delay(1000);  // Increase delay to 1 second for better readability
}
