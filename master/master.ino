#include <esp_now.h>
#include <WiFi.h>

#define BUTTON1_PIN 5  // D4 on most ESP32 boards
#define BUTTON2_PIN 6
#define BUTTON3_PIN 7

uint8_t slaveA[] = {0x8C, 0xD0, 0xB2, 0xA9, 0xE7, 0x6A};  // MAC của slave 8C:D0:B2:A9:E7:6A
uint8_t slaveB[] = {0x8C, 0xD0, 0xB2, 0xA8, 0x10, 0xE1};  // 8C:D0:B2:A8:10:E1
uint8_t slaveSwitchServo[] = {0xE4, 0xB3, 0x23, 0xC6, 0x81, 0x58};  // E4:B3:23:C6:81:58

bool lastButton1 = HIGH;
bool lastButton2 = HIGH;
bool lastButton3 = HIGH;

void onSendComplete(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.print("[ESP-NOW] Send to ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.print(" -> ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ----------------- ADD PEER FUNCTION -----------------
void addPeer(uint8_t *address) {
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, address, 6);
  peer.channel = 0;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Failed to add peer!");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onSendComplete);

  // Add both slaves
  addPeer(slaveA);
  addPeer(slaveB);
  addPeer(slaveSwitchServo);

  Serial.println("Master ready");
}

void loop() {
  // Read button inputs
  bool b1 = digitalRead(BUTTON1_PIN);
  bool b2 = digitalRead(BUTTON2_PIN);
  bool b3 = digitalRead(BUTTON3_PIN);

  // ----------------- BUTTON 1 → SLAVE A -----------------
  if (b1 != lastButton1) {
    lastButton1 = b1;
    uint8_t value = (b1 == LOW) ? 1 : 0;
    esp_now_send(slaveA, &value, sizeof(value));
    Serial.printf("[SEND] Button1 %s → Slave A (value=%d)\n",
                  b1 == LOW ? "Pressed" : "Released",
                  value);
    delay(80); // debounce
  }

  // ----------------- BUTTON 2 → SLAVE B -----------------
  if (b2 != lastButton2) {
    lastButton2 = b2;
    uint8_t value = (b2 == LOW) ? 1 : 0;
    esp_now_send(slaveB, &value, sizeof(value));
    Serial.printf("[SEND] Button2 %s → Slave B (value=%d)\n",
                  b2 == LOW ? "Pressed" : "Released",
                  value);
    delay(80); // debounce
  }

  // ----------------- BUTTON 3 → SLAVE SERVO SWITCH -----------------
  if (b3 != lastButton3) {
    lastButton3 = b3;
    uint8_t value = (b3 == LOW) ? 1 : 0;
    esp_now_send(slaveSwitchServo, &value, sizeof(value));
    Serial.printf("[SEND] Button3 %s → Slave Switch Servo (value=%d)\n",
                  b2 == LOW ? "Pressed" : "Released",
                  value);
    delay(80); // debounce
  }

  delay(10);
}


