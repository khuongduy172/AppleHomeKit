/*
 * rf-slave.ino  —  ESP32-C3 Super Mini + nRF24L01  (DEEP SLEEP receiver)
 *
 * Low-power RF receiver that drives a GPIO output (relay / LED / etc).
 *
 * How the deep sleep works
 * ------------------------
 * The ESP32-C3 cannot receive RF while in deep sleep (CPU + WiFi/SPI are off).
 * So we let the nRF24L01 do the listening: it stays in RX mode and pulls its
 * IRQ pin LOW the moment a packet arrives. That LOW edge is configured as the
 * ESP32-C3 deep-sleep GPIO wake source. Sequence on every packet:
 *
 *   deep sleep (~5uA on the ESP)  ->  nRF gets packet, IRQ goes LOW
 *     ->  ESP wakes, runs setup(), reads the command, sets the GPIO
 *     ->  GPIO state is HELD across sleep, IRQ is cleared, ESP sleeps again
 *
 * Power note: the nRF in RX draws ~13.5mA continuously - that is the price of
 * being able to receive at any instant. See the comment block at the bottom
 * for a much lower-power (but higher-latency) timer-polling alternative.
 *
 * IMPORTANT: on the ESP32-C3 only GPIO0..GPIO5 can wake from deep sleep, so the
 * nRF IRQ line must connect to one of those pins (GPIO3 here).
 *
 * Library: "RF24" by TMRh20
 *
 * Wiring  nRF24L01  ->  ESP32-C3 Super Mini
 *   GND   -> GND
 *   VCC   -> 3V3  (10uF cap across VCC/GND right at the module is essential)
 *   CE    -> GPIO10
 *   CSN   -> GPIO7
 *   SCK   -> GPIO4
 *   MOSI  -> GPIO6
 *   MISO  -> GPIO5
 *   IRQ   -> GPIO3   (deep-sleep wake pin, active LOW)
 *
 *   OUTPUT_PIN (GPIO1) -> relay / LED / load
 */

#include <SPI.h>
#include <RF24.h>
#include "esp_sleep.h"
#include "driver/gpio.h"

// ---------- nRF24 pins ----------
#define PIN_CE    10
#define PIN_CSN   7
#define PIN_SCK   4
#define PIN_MISO  5
#define PIN_MOSI  6
#define PIN_IRQ   GPIO_NUM_3     // must be GPIO0..5 on the C3

// ---------- outputs ----------
// Two momentary outputs (e.g. motor up / down, or ON / OFF push lines).
#define OUTPUT_PIN  GPIO_NUM_1    // pulsed when cmd = ON  (avoid strapping pins 2/8/9)
#define OUTPUT_PIN1 GPIO_NUM_0    // pulsed when cmd = OFF
#define PULSE_MS    300          // how long each pulse stays HIGH

// Must match the master exactly
const uint8_t RF_ADDRESS[6] = "NODE1";
const uint8_t RF_CHANNEL    = 76;

struct Payload {
  uint8_t magic;   // 0xA5
  uint8_t cmd;     // 1 = ON, 0 = OFF
};

RF24 radio(PIN_CE, PIN_CSN);

// Low-level momentary pulse on a pin. Releases the deep-sleep hold first so the
// pin can change, drives it HIGH for PULSE_MS, then back LOW.
void pulseOutput(gpio_num_t pin) {
  gpio_hold_dis(pin);
  gpio_set_level(pin, 1);
  delay(PULSE_MS);
  gpio_set_level(pin, 0);
}

// =============================================================================
//  >>> EDIT HERE: define WHAT happens on a received message <<<
//  Called once with a valid payload. Decide what the GPIO(s) should do.
//  Add pins, invert logic, decode extra fields from the payload, etc.
// =============================================================================
void handleCommand(const Payload &p) {
  bool state = (p.cmd == 1);
  if (state) {
    pulseOutput(OUTPUT_PIN);    // cmd = ON
  } else {
    pulseOutput(OUTPUT_PIN1);   // cmd = OFF
  }
  Serial.printf("[RECV] cmd = %s -> pulsed GPIO%d\n",
                state ? "ON" : "OFF", state ? OUTPUT_PIN : OUTPUT_PIN1);
}
// =============================================================================

void goToDeepSleep() {
  // Drain any pending RX so the IRQ line returns HIGH; otherwise the ESP would
  // wake immediately on the still-asserted (LOW) IRQ.
  while (radio.available()) {
    Payload junk;
    radio.read(&junk, sizeof(junk));
  }
  bool tx_ok, tx_fail, rx_ready;
  radio.whatHappened(tx_ok, tx_fail, rx_ready);  // clears nRF IRQ flags

  // Latch both output pins LOW so they stay idle while the ESP is asleep.
  gpio_hold_en(OUTPUT_PIN);
  gpio_hold_en(OUTPUT_PIN1);
  gpio_deep_sleep_hold_en();

  // Wake when the nRF pulls IRQ LOW (new packet received).
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_IRQ, ESP_GPIO_WAKEUP_GPIO_LOW);

  Serial.println("Sleeping... (nRF listening, ESP in deep sleep)");
  Serial.flush();
  esp_deep_sleep_start();   // never returns; chip resets into setup() on wake
}

void setup() {
  Serial.begin(115200);
  delay(50);

  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.printf("\nBoot. wake cause = %d (%s)\n", cause,
                cause == ESP_SLEEP_WAKEUP_GPIO ? "packet" : "power-on/reset");

  // ---- output pins ----
  // Both are momentary: they idle LOW and only pulse HIGH inside handleCommand().
  gpio_hold_dis(OUTPUT_PIN);
  gpio_hold_dis(OUTPUT_PIN1);
  gpio_reset_pin(OUTPUT_PIN);
  gpio_reset_pin(OUTPUT_PIN1);
  gpio_set_direction(OUTPUT_PIN,  GPIO_MODE_OUTPUT);
  gpio_set_direction(OUTPUT_PIN1, GPIO_MODE_OUTPUT);
  gpio_set_level(OUTPUT_PIN,  0);
  gpio_set_level(OUTPUT_PIN1, 0);

  // ---- radio ----
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CSN);
  if (!radio.begin(&SPI)) {
    Serial.println("nRF24 not responding - check wiring/power!");
    // Sleep a bit and reset to retry instead of busy-looping at full power.
    esp_sleep_enable_timer_wakeup(2 * 1000000ULL);
    esp_deep_sleep_start();
  }

  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(RF_CHANNEL);
  radio.setPayloadSize(sizeof(Payload));
  radio.openReadingPipe(1, RF_ADDRESS);

  // Only assert the IRQ pin on "data received"; mask TX events.
  radio.maskIRQ(true, true, false);  // (tx_ok, tx_fail, rx_ready) true = masked

  radio.startListening();

  // Read whatever woke us. The master sends a retry burst, so act on the FIRST
  // valid packet only (the pulse is momentary - we don't want to fire it twice).
  // goToDeepSleep() drains any remaining duplicates afterwards.
  unsigned long start = millis();
  bool got = false;
  Payload p;
  while (millis() - start < 50) {     // short window to catch the burst
    if (radio.available()) {
      radio.read(&p, sizeof(p));
      if (p.magic == 0xA5) {
        handleCommand(p);
        got = true;
        break;
      }
    }
  }
  if (!got && cause == ESP_SLEEP_WAKEUP_GPIO) {
    Serial.println("[RECV] woke but no valid payload (noise?)");
  }

  goToDeepSleep();
}

void loop() {
  // unused - everything happens in setup() because deep sleep resets the chip
}

/*
 * -----------------------------------------------------------------------------
 * LOWER-POWER ALTERNATIVE (timer polling)
 * -----------------------------------------------------------------------------
 * If average current must be in the microamp range and a 1-2s response delay is
 * acceptable, power the nRF down too and wake the ESP on a timer instead of IRQ:
 *
 *   1. In goToDeepSleep(): call radio.powerDown();  (nRF -> ~900nA)
 *      and replace the GPIO wake line with:
 *        esp_sleep_enable_timer_wakeup(1 * 1000000ULL);   // wake every 1s
 *   2. In setup(): radio.powerUp(); radio.startListening();
 *      then listen for ~10-20ms for a packet before sleeping again.
 *   3. On the master, send the command repeatedly for >1s so it overlaps one of
 *      the receiver's listen windows.
 *
 * This trades instant response for far lower average power.
 * -----------------------------------------------------------------------------
 */
