struct DEV_RELAY : Service::Switch {
  int relayPin;
  bool powerOn = false;
  SpanCharacteristic *power;
  DEV_RELAY(int replayPin) : Service::Switch() {
    this->relayPin = replayPin;
    pinMode(replayPin, OUTPUT);
    digitalWrite(replayPin, HIGH);
    power = new Characteristic::On(powerOn);
  }

  boolean update() {
    digitalWrite(relayPin, power->getNewVal() ? LOW : HIGH);

    return (true);
  }
};