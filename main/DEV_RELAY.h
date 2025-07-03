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
    boolean powerValue = power->getNewVal() ? true : false;
    digitalWrite(relayPin, powerValue ? HIGH : LOW);
    
    power->setValidValues(powerValue);
    return (true);
  }
};