#include <WiFi.h>
#include <esp_now.h>

#define LED_PIN 2
#define LED_PIN1 1

void onReceive(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  const uint8_t *mac = recv_info->src_addr;
  Serial.printf("Received %d bytes from %02X:%02X:%02X:%02X:%02X:%02X\n",
                len,
                mac[0], mac[1], mac[2],
                mac[3], mac[4], mac[5]);

  if (len == 1) {
    bool state = incomingData[0];
    if (state) {
      digitalWrite(LED_PIN, HIGH);
      delay(300);
      digitalWrite(LED_PIN, LOW);
    } else {
      digitalWrite(LED_PIN1, HIGH);
      delay(300);
      digitalWrite(LED_PIN1, LOW);
    }
    
    Serial.printf("Received from Master: %s\n", state ? "ON" : "OFF");
  }
}

void setup() {
  Serial.begin(115200);
  // Serial.setTxTimeoutMs(0);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN1, OUTPUT);

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
