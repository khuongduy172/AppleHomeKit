struct DEV_RELAY : Service::Switch {
  int relayPin;
  int powerPin;
  bool powerOn = false;
  SpanCharacteristic *power;
  DEV_RELAY(int replayPin, int powerPin) : Service::Switch() {
    this->relayPin = replayPin;
    pinMode(replayPin, OUTPUT);

    power = new Characteristic::On(powerOn);
    digitalWrite(replayPin, HIGH);

    new SpanButton(powerPin);
    this->powerPin = powerPin;
  }

  boolean update() {
    toggleRelay();
    return (true);
  }

  void toggleRelay(bool isFromButton = false) {
    this->powerOn = !this->powerOn;
    digitalWrite(relayPin, powerOn ? HIGH : LOW);
    Serial.print("Relay state changed to: ");
    Serial.println(powerOn ? "ON" : "OFF");
    if (isFromButton) {
      power->setVal(powerOn);
    }
    
    power->setValidValues(powerOn);
  }

  void button(int pin, int pressType) override {
    if(pin==powerPin && pressType==SpanButton::SINGLE){
      toggleRelay(true);
    }
  }
};