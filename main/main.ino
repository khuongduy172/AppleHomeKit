#include "HomeSpan.h"
#include "DEV_RELAY.h"

void setup() {
  Serial.begin(115200);
  homeSpan.setPairingCode("11122333");

  homeSpan.setControlPin(15);
  homeSpan.setStatusPin(2);

  homeSpan.setApTimeout(300);
  homeSpan.enableAutoStartAP();
  
  homeSpan.begin(Category::Bridges, "Monitor Light");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Front Light");
    new DEV_RELAY(12);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Black Light");
    new DEV_RELAY(14);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Flex1");
    new DEV_RELAY(27);

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Flex2");
    new DEV_RELAY(26);
}
void loop() {
  homeSpan.poll();
}