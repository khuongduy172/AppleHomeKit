#include <esp_now.h>
#include <WiFi.h>

#define BUTTON_PIN 4  // D4 on most ESP32 boards

uint8_t slaveAddress[] = {0xF4, 0x65, 0x0B, 0xEA, 0xC7, 0x84};  // MAC của slave

bool lastButtonState = HIGH;

void onSendComplete(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.print("[ESP-NOW] Send to ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.print(" -> ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onSendComplete);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, slaveAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("Master ready");
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  // Gửi khi nút chuyển trạng thái
  if (buttonState != lastButtonState) {
    lastButtonState = buttonState;
    uint8_t value = (buttonState == LOW) ? 1 : 0;
    esp_now_send(slaveAddress, &value, sizeof(value));
    Serial.printf("[SEND] Button %s -> Sent value: %d\n", buttonState == LOW ? "Pressed" : "Released", value);
    delay(50);  // debounce
  }

  delay(10);
}
