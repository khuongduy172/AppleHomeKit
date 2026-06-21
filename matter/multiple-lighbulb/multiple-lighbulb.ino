// Matter Manager Library
#include <Matter.h>
#include <DHT.h>

// Accessory Structure for dynamic management
struct LightAccessory {
  MatterOnOffLight* endpoint;
  uint8_t pin;
  const char* name;
};

// List of Matter Endpoints for this Node
#define MAX_LIGHTS 1
LightAccessory lights[MAX_LIGHTS] = {
  {new MatterOnOffLight(), 8, "Light 1"},
};

// Temperature and Humidity Sensor Endpoint
MatterTemperatureSensor TempSensor;
MatterHumiditySensor HumiditySensor;

// DHT11 Sensor
const uint8_t dhtPin = 5;
const uint8_t dhtType = DHT11;
DHT dhtSensor(dhtPin, dhtType);

// Sensor reading interval (5000ms)
const uint32_t sensorReadInterval = 30000;
uint32_t lastSensorReadTime = 0;
float lastTemperature = 0.0;
float lastHumidity = 0.0;

// set your board USER BUTTON pin here - decommissioning button
const uint8_t buttonPin = 4;  // Set your pin here. Using BOOT Button.

// Button control - decommision the Matter Node
uint32_t button_time_stamp = 0;                // debouncing control
bool button_state = false;                     // false = released | true = pressed
const uint32_t decommissioningTimeout = 5000;  // keep the button pressed for 5s, or longer, to decommission

// Dynamic Light Control Callback
bool handleLightCallback(int lightIndex, bool state) {
  if (lightIndex >= 0 && lightIndex < MAX_LIGHTS) {
    digitalWrite(lights[lightIndex].pin, state ? LOW : HIGH);
    Serial.println((String)lights[lightIndex].name + " State - " + (state ? "ON" : "OFF"));
    return true;
  }
  return false;
}

// Callback wrappers for each light (required by Matter library)
bool onOffLightCallback(bool state) {
  return handleLightCallback(0, state);
}

void setup() 
{
  Serial.begin(115200);

  // Initialize the USER BUTTON (Boot button) that will be used to decommission the Matter Node
  pinMode(buttonPin, INPUT_PULLUP);
  
  // Initialize all light GPIOs dynamically
  for (int i = 0; i < MAX_LIGHTS; i++) {
    pinMode(lights[i].pin, OUTPUT);
    Serial.println((String)lights[i].name + " initialized on pin " + (String)lights[i].pin);
  }

  // Initialize DHT11 sensor
  dhtSensor.begin();

  // Initialize all light endpoints dynamically
  for (int i = 0; i < MAX_LIGHTS; i++) {
    lights[i].endpoint->begin();
  }
  
  // Initialize temperature and humidity endpoints
  TempSensor.begin();
  HumiditySensor.begin();

  // Register callbacks for lights
  lights[0].endpoint->onChange(onOffLightCallback);

  // Matter beginning - Last step, after all EndPoints are initialized
  Matter.begin();

  if (!Matter.isDeviceCommissioned()) {
    Serial.println("Matter Node is not commissioned yet.");
    Serial.println("Initiate the device discovery in your Matter environment.");
    Serial.println("Commission it to your Matter hub with the manual pairing code or QR code");
    Serial.printf("Manual pairing code: %s\r\n", Matter.getManualPairingCode().c_str());
    Serial.printf("QR code URL: %s\r\n", Matter.getOnboardingQRCodeUrl().c_str());
  }
}

void loop() {
  // Check if the button has been pressed
  if (digitalRead(buttonPin) == LOW && !button_state) {
    // deals with button debouncing
    button_time_stamp = millis();  // record the time while the button is pressed.
    button_state = true;           // pressed.
  }

  if (digitalRead(buttonPin) == HIGH && button_state) {
    button_state = false;  // released
  }

  // Onboard User Button is kept pressed for longer than 5 seconds in order to decommission matter node
  uint32_t time_diff = millis() - button_time_stamp;
  if (button_state && time_diff > decommissioningTimeout) {
    Serial.println("Decommissioning the Light Matter Accessory. It shall be commissioned again.");
    Matter.decommission();
    button_time_stamp = millis();  // avoid running decommissining again, reboot takes a second or so
  }

  // Read sensor data at 5000ms interval
  if (millis() - lastSensorReadTime >= sensorReadInterval) {
    lastSensorReadTime = millis();
    
    // Read temperature and humidity from DHT11
    lastTemperature = dhtSensor.readTemperature();
    lastHumidity = dhtSensor.readHumidity();
    
    // Check if reading is valid
    if (!isnan(lastTemperature) && !isnan(lastHumidity)) {
      Serial.println("Temperature: " + (String)lastTemperature + "°C, Humidity: " + (String)lastHumidity + "%");
      // Update Matter sensors
      TempSensor.setTemperature(lastTemperature);
      HumiditySensor.setHumidity(lastHumidity);
    } else {
      Serial.println("Failed to read from DHT11 sensor");
    }
  }

  delay(500);
}