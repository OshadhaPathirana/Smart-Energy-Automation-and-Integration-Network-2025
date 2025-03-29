/**
 * QueryAggregated Example code for InfluxDBClient library for Arduino.
 * 
 * This example demonstrates querying basic aggregated statistic parameters of WiFi signal level measured and stored in BasicWrite and SecureWrite examples.
 * 
 * Demonstrates connection to any InfluxDB instance accesible via:
 *  - unsecured http://...
 *  - secure https://... (appropriate certificate is required)
 *  - InfluxDB 2 Cloud at https://cloud2.influxdata.com/ (certificate is preconfigured)
 * 
 *  Enter WiFi and InfluxDB parameters below
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
 
 // WiFi AP SSID
 #define WIFI_SSID "Dialog 4G 139"
 // WiFi password
 #define WIFI_PASSWORD "dBb809D3"
 // InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
 // InfluxDB 1.8+  (v2 compatibility API) server url, e.g. http://192.168.1.48:8086
 #define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com"
 // InfluxDB v2 server or cloud API authentication token (Use: InfluxDB UI -> Load Data -> Tokens -> <select token>)
 // InfluxDB 1.8+ (v2 compatibility API) use form user:password, eg. admin:adminpass
 #define INFLUXDB_TOKEN "kBLLkVYJr_s0oz0bqfnZDFk8AH-Kp8P56uwqWeZWfgJs7WV1HfHknQpyDMiXJbsqaReU_AOxtdq2MB6_sFol7w=="
 // InfluxDB v2 organization name or id (Use: InfluxDB UI -> Settings -> Profile -> <name under tile> )
 // InfluxDB 1.8+ (v2 compatibility API) use any non empty string
 #define INFLUXDB_ORG "8016ae0a3fa30f7e"
 // InfluxDB v2 bucket name (Use: InfluxDB UI -> Load Data -> Buckets)
 // InfluxDB 1.8+ (v2 compatibility API) use database name
 #define INFLUXDB_BUCKET "Bucket1"
 
 // Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
 // Examples:
 //  Pacific Time: "PST8PDT"
 //  Eastern: "EST5EDT"
 //  Japanesse: "JST-9"
 //  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
 #define TZ_INFO "UTC5.5"
 
 // InfluxDB client instance with preconfigured InfluxCloud certificate
 InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
 
 void setup() {
   Serial.begin(115200);
 
   // Setup wifi
   WiFi.mode(WIFI_STA);
   wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
 
   Serial.print("Connecting to wifi");
   while (wifiMulti.run() != WL_CONNECTED) {
     Serial.print(".");
     delay(500);
   }
   Serial.println();
 
 
   // Accurate time is necessary for certificate validation
   // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
   // Syncing progress and the time will be printed to Serial
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
   printAggregatedVoltage();
 
   //Wait 10s
   Serial.println("Wait 1s");
   delay(1000);
 }
 
 // printAgregateResult queries db for aggregated RSSI value computed by given InfluxDB selector function (max, mean, min)
 // Prints composed query and the result values.
 void printAggregatedVoltage() {
   // Construct Flux query
   String query =
     "from(bucket: \"" INFLUXDB_BUCKET "\") "
     "|> range(start: -2s) "
     "|> filter(fn: (r) => r[\"_measurement\"] == \"Inverters\") "
     "|> filter(fn: (r) => r[\"_field\"] == \"Voltage\") "
     "|> aggregateWindow(every: 1s, fn: mean, createEmpty: false) "
     "|> yield(name: \"mean\")";
   
   // Print query details
   Serial.println("==== Querying Aggregated Voltage ====");
   Serial.print("Query: ");
   Serial.println(query);
 
   // Send query and get results
   FluxQueryResult result = client.query(query);
 
   // Iterate over results
   while (result.next()) {
     double value = result.getValueByName("_value").getDouble();
     FluxDateTime time = result.getValueByName("_time").getDateTime();
 
     Serial.print("Mean Voltage: ");
     Serial.print(value, 2);  // Print with 2 decimal places
     Serial.print("V at ");
     Serial.println(time.format("%F %T"));
   }
 
   // Check for errors
   if (result.getError() != "") {
     Serial.print("Query Error: ");
     Serial.println(result.getError());
   }
 
   // Close the result
   result.close();
 }
 