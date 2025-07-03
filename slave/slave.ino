#include <WiFi.h>
#include <esp_now.h>

#define LED_PIN 14

void onReceive(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.printf("Received %d bytes from %02X:%02X:%02X:%02X:%02X:%02X\n",
                len,
                mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5]);

  if (len == 1) {
    bool state = incomingData[0];
    digitalWrite(LED_PIN, state ? HIGH : LOW);
    Serial.printf("Received from Master: %s\n", state ? "ON" : "OFF");
  }
}

void setup() {
  Serial.begin(115200);
  // Serial.setTxTimeoutMs(0);
  pinMode(LED_PIN, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Không kết nối Wi-Fi

  // In ra địa chỉ MAC để lấy dán vào Master
  Serial.print("Slave MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Slave Mode: ");
  Serial.println(WiFi.getMode());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
}

void loop() {
  // Không cần xử lý gì thêm
}
