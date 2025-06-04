/**
 * QueryAggregated Example code for InfluxDBClient library for Arduino.
 * 
 * This example demonstrates querying aggregated Voltage and Frequency values from InfluxDB.
 **/

 #if defined(ESP32)
 #include <WiFiMulti.h>
 WiFiMulti wifiMulti;
 #define DEVICE "ESP32"
 #elif defined(ESP8266)
 #include <ESP8266WiFiMulti.h>
 ESP8266WiFiMulti wifiMulti;
 #define DEVICE "ESP8266"
 #endif
 
 #include <InfluxDbClient.h>
 #include <InfluxDbCloud.h>
 
 // WiFi credentials
 #define WIFI_SSID "Dialog 4G 139"
 #define WIFI_PASSWORD "dBb809D3"
 
 // InfluxDB configuration
 #define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
 #define INFLUXDB_TOKEN "kBLLkVYJr_s0oz0bqfnZDFk8AH-Kp8P56uwqWeZWfgJs7WV1HfHknQpyDMiXJbsqaReU_AOxtdq2MB6_sFol7w=="
 #define INFLUXDB_ORG "8016ae0a3fa30f7e"
 #define INFLUXDB_BUCKET "Bucket1"
 #define TZ_INFO "UTC5.5"
 
 // InfluxDB client instance with preconfigured InfluxCloud certificate
 InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
 
 void setup() {
   Serial.begin(115200);
 
   // Setup WiFi
   WiFi.mode(WIFI_STA);
   wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
 
  // Serial.print("Connecting to WiFi");
   while (wifiMulti.run() != WL_CONNECTED) {
     //Serial.print(".");
     delay(500);
   }
   //Serial.println("\nConnected!");
 
   // Sync time for certificate validation
   timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
 
   /* // Check InfluxDB connection
   if (client.validateConnection()) {
     Serial.print("Connected to InfluxDB: ");
     Serial.println(client.getServerUrl());
   } else {
     Serial.print("InfluxDB connection failed: ");
     Serial.println(client.getLastErrorMessage());
   }*/
 }
 
 void loop() {
   printAggregatedData();
   //Serial.println("Wait 1s");
   delay(1000);
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
 
   //Serial.println("==== Querying Aggregated Data ====");
   //Serial.println("Query: " + query);
 
   FluxQueryResult result = client.query(query);
 
   // Process query results
   while (result.next()) {
     String field = result.getValueByName("_field").getString();
     double value = result.getValueByName("_value").getDouble();
     FluxDateTime time = result.getValueByName("_time").getDateTime();
 
     //Serial.print(field + ": ");
     if (field == "Frequency") {
       Serial.print("F");
       Serial.print(value / 1000, 2);
       Serial.print(" ");
     } else {
       Serial.print("A");
       Serial.println(value / 1000, 2);
     }
 
     //Serial.print(" at ");
     //Serial.println(time.format("%F %T"));
   }
 
   // Check for errors
   /*if (result.getError() != "") {
     Serial.print("Query Error: ");
     Serial.println(result.getError());
   }*/
 
   result.close();
 }
 