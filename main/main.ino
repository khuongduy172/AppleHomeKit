#include "HomeSpan.h"
#include "DEV_RELAY.h"

#define NUM_SWITCHES 4

int relayPins[NUM_SWITCHES] = {12, 14, 27, 26};
const char* switchesName[NUM_SWITCHES] = {"Front Light", "Black Light", "Flex1", "Flex2"};

DEV_RELAY* switches[NUM_SWITCHES];

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

  for (int i = 0; i < NUM_SWITCHES; i++) {
    new SpanAccessory();
    new Service::AccessoryInformation();
    new Characteristic::Identify();
    new Characteristic::Name(switchesName[i]);
    switches[i] = new DEV_RELAY(relayPins[i]);
  }
}
void loop() {
  homeSpan.poll();
}