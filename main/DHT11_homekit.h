#pragma once

#include "HomeSpan.h"
#include <DHT.h>

// -------- CONFIG --------
#define DHTPIN   21
#define DHTTYPE  DHT11
#define DHT_READ_INTERVAL 5000  // ms

// -------- GLOBAL DHT OBJECT --------
static DHT dht(DHTPIN, DHTTYPE);

// -------- TEMPERATURE SERVICE --------
struct TempSensor : Service::TemperatureSensor {

  SpanCharacteristic *tempChar;
  unsigned long lastRead = 0;

  TempSensor() {
    tempChar = new Characteristic::CurrentTemperature(0);
    tempChar->setRange(-20, 60);   // HomeKit required range
  }

  void loop() override {
    if (millis() - lastRead >= DHT_READ_INTERVAL) {
      lastRead = millis();

      float t = dht.readTemperature(); // Celsius
      if (!isnan(t)) {
        tempChar->setVal(t);
      }
    }
  }
};

// -------- HUMIDITY SERVICE --------
struct HumiditySensor : Service::HumiditySensor {

  SpanCharacteristic *humChar;
  unsigned long lastRead = 0;

  HumiditySensor() {
    humChar = new Characteristic::CurrentRelativeHumidity(0);
  }

  void loop() override {
    if (millis() - lastRead >= DHT_READ_INTERVAL) {
      lastRead = millis();

      float h = dht.readHumidity();
      if (!isnan(h)) {
        humChar->setVal(h);
      }
    }
  }
};

// -------- INIT FUNCTION --------
inline void initDHT11() {
  dht.begin();
}

// -------- ACCESSORY BUILDER --------
inline void createDHT11Accessory() {

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Temp & Humid");

    new TempSensor();
    new HumiditySensor();
}
