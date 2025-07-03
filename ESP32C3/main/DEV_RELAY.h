struct DEV_RELAY : Service::Switch {
  int relayPin;
  SpanCharacteristic *power;
  DEV_RELAY(int replayPin) : Service::Switch() {
    this->relayPin = replayPin;
    pinMode(replayPin, OUTPUT);

    power = new Characteristic::On(true);
    digitalWrite(replayPin, HIGH);
  }

  boolean update() {
    digitalWrite(relayPin, power.getNewVal() ? HIGH : LOW);
    
    power->setValidValues(powerOn);
    return (true);
  }
};