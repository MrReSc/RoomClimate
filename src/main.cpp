#include "DHT.h"
#include "Wire.h"
#include "WiFi.h"
#include <InfluxDbClient.h>
#include "SparkFunCCS811.h" //https://learn.sparkfun.com/tutorials/ccs811-air-quality-breakout-hookup-guide#introduction

#define DHTPIN 4        // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

#define CCS811_ADDR 0x5B //Default I2C Address
CCS811 myCCS811(CCS811_ADDR);

const char* ssid = "*****";        //SSID
const char* password =  "*****";   //Password

#define INFLUXDB_URL "http://192.168.0.2:9086"
#define INFLUXDB_TOKEN "*****"    //Token
#define INFLUXDB_ORG "roomClimateData"
#define INFLUXDB_BUCKET "roomClimate"
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point pointDevice("roomClimate");

void setup() {
  Serial.begin(9600);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nis.gov");

  dht.begin();
  Wire.begin();
  myCCS811.begin();
}

void loop() {
  // Wait a few seconds between measurements.
  delay(5000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.println(t);

  pointDevice.addField("temp", t);
  pointDevice.addField("hum", h);
 
  if (myCCS811.dataAvailable())
  {
    myCCS811.setEnvironmentalData(h, t);
    myCCS811.readAlgorithmResults();

    float co2 = myCCS811.getCO2();
    float tvoc = myCCS811.getTVOC();

    Serial.print("CO2:");
    Serial.print(co2);
    Serial.print(" tVOC:");
    Serial.println(tvoc);

    pointDevice.addField("co2", co2);
    pointDevice.addField("tvoc", tvoc);
  }

  // Write data
  client.writePoint(pointDevice);
}