/*
 * master-rf.ino  —  ESP32-C3 Super Mini + nRF24L01
 *
 * Sends an ON/OFF command over RF to the receiver (rf-slave).
 * Trigger by either:
 *   - pressing the BOOT button (GPIO9) -> toggles ON/OFF
 *   - typing '1' (on), '0' (off) or 't' (toggle) in the Serial Monitor
 *
 * Library: "RF24" by TMRh20 (install via Library Manager)
 *
 * Wiring  nRF24L01  ->  ESP32-C3 Super Mini
 *   GND   -> GND
 *   VCC   -> 3V3  (add a 10uF cap across VCC/GND close to the module!)
 *   CE    -> GPIO10
 *   CSN   -> GPIO7
 *   SCK   -> GPIO4
 *   MOSI  -> GPIO6
 *   MISO  -> GPIO5
 *   IRQ   -> (not used on the master)
 */

#include <SPI.h>
#include <RF24.h>

// ---------- nRF24 pins ----------
#define PIN_CE    10
#define PIN_CSN   7
#define PIN_SCK   4
#define PIN_MISO  5
#define PIN_MOSI  6

#define BUTTON_PIN 9   // BOOT button on the Super Mini

// Must match the receiver exactly
const uint8_t RF_ADDRESS[6] = "NODE1";
const uint8_t RF_CHANNEL    = 76;

// Shared payload (keep identical on both sides)
struct Payload {
  uint8_t magic;   // 0xA5 -> sanity check so RF noise is ignored
  uint8_t cmd;     // 1 = ON, 0 = OFF
};

RF24 radio(PIN_CE, PIN_CSN);

bool outputState = false;
bool lastButton  = HIGH;

// =============================================================================
//  >>> EDIT HERE: define WHAT you send <<<
//  Build the payload for a given on/off request. Add fields, change values,
//  encode brightness/channel/etc. - just keep the struct in sync with the slave.
// =============================================================================
Payload buildPayload(bool on) {
  Payload p;
  p.magic = 0xA5;
  p.cmd   = on ? 1 : 0;
  return p;
}
// =============================================================================

// RF transport - usually no need to touch this.
void sendCommand(bool on) {
  Payload p = buildPayload(on);

  radio.stopListening();              // TX mode
  bool ok = radio.write(&p, sizeof(p));

  Serial.printf("[SEND] %s -> %s\n", on ? "ON" : "OFF",
                ok ? "ACK received" : "no ACK (retrying possible)");

  // A sleeping receiver can miss the first packet while it wakes up.
  // Burst a few copies so the command reliably lands.
  for (int i = 0; i < 3 && !ok; i++) {
    delay(5);
    ok = radio.write(&p, sizeof(p));
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CSN);

  if (!radio.begin(&SPI)) {
    Serial.println("nRF24 not responding - check wiring/power!");
    while (true) delay(1000);
  }

  radio.setPALevel(RF24_PA_LOW);     // raise to RF24_PA_HIGH for more range
  radio.setDataRate(RF24_250KBPS);   // lowest rate = best sensitivity/range
  radio.setChannel(RF_CHANNEL);
  radio.setPayloadSize(sizeof(Payload));
  radio.setRetries(5, 15);           // 5*250us delay, 15 retries
  radio.enableDynamicAck();
  radio.openWritingPipe(RF_ADDRESS);
  radio.stopListening();

  Serial.println("Master ready. Button=toggle, Serial: '1' on / '0' off / 't' toggle");
}

void loop() {
  // ---- button (toggle on press) ----
  bool b = digitalRead(BUTTON_PIN);
  if (b != lastButton) {
    lastButton = b;
    if (b == LOW) {                  // pressed
      outputState = !outputState;
      sendCommand(outputState);
    }
    delay(50);                       // debounce
  }

  // ---- serial control ----
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '1') { outputState = true;  sendCommand(true);  }
    else if (c == '0') { outputState = false; sendCommand(false); }
    else if (c == 't') { outputState = !outputState; sendCommand(outputState); }
  }

  delay(10);
}
