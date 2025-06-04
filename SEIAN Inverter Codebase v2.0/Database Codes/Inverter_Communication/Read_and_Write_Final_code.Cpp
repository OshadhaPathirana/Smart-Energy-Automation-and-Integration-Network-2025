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

#define WIFI_SSID "Dialog 4G 139"  // Network Name
#define WIFI_PASSWORD "dBb809D3"
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
}

void loop() {
  // Read sensor data and write to InfluxDB
  if (jsy.read()) {
    float v_1 = jsy.data.channel1().voltage;
    float i_1 = jsy.data.channel1().current;
    float f_1 = jsy.data.channel1().frequency;

    int v1 = v_1 * 1000;  // Converting floats to integers, multiplied by 1000 for InfluxDB write
    int f1 = f_1 * 1000;
    int i1 = i_1 * 1000;

    // Clear fields for reusing the point. Tags will remain the same as set above.
    sensor.clearFields();

    // Store measured value into point
    sensor.addField("Voltage", v1);
    sensor.addField("Current", i1);
    sensor.addField("Frequency", f1);

    // Print what we are writing to InfluxDB
   // Serial.print("Writing to InfluxDB: ");
    sensor.toLineProtocol();
    //Serial.println(sensor.toLineProtocol());
  }

  // Write point to InfluxDB
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // Query aggregated data from InfluxDB
  printAggregatedData();

  //Serial.println("Waiting 1 second");
  delay(1000);
}
double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
void printAggregatedData() {
  // Query for both Voltage and Frequency
  String query =
    "from(bucket: \"" INFLUXDB_BUCKET "\") "
    "|> range(start: -2s) "
    "|> filter(fn: (r) => r[\"_measurement\"] == \"Inverters\") "
    "|> filter(fn: (r) => r[\"_field\"] == \"Voltage\" or r[\"_field\"] == \"Frequency\") "
    "|> aggregateWindow(every: 1s, fn: mean, createEmpty: false) "
    "|> yield(name: \"mean\")";

  // Query the data
  FluxQueryResult result = client.query(query);

  // Process query results
  while (result.next()) {
    String field = result.getValueByName("_field").getString();
    double value = result.getValueByName("_value").getDouble();
    FluxDateTime time = result.getValueByName("_time").getDateTime();

    // Print the queried data to Serial
    if (field == "Frequency") {
      //Serial.print("F");
      
      //Serial.print(value / 1000, 2);
      Serial.print("");
    } else {
      Serial.print("A");
      value = value/1000;
      double mappedValue = mapDouble(value, 0.0, 230.0, 0.0, 1023.0);
      Serial.println(mappedValue, 2);
    }
  }

  result.close();
}
