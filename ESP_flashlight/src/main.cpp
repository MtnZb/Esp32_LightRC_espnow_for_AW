#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

const int flashlightPin = LED_BUILTIN;
typedef struct struct_message {
  bool Light_On;
} __attribute__((packed)) struct_message;
struct_message incomingMessage;

// put function declarations here:
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len);

void setup() {
  Serial.begin(115200);
  pinMode(flashlightPin, OUTPUT);
  digitalWrite(flashlightPin, HIGH);  // Изначально выключаем фонарик (на D1 Mini HIGH выключает встроенный светодиод) не забудьте убрать инверсию логики

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println();
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incomingMessage, incomingData, sizeof(incomingMessage));

  if (incomingMessage.Light_On) {
    digitalWrite(flashlightPin, LOW);  // Включаем фонарик (на D1 Mini LOW включает встроенный светодиод) с инверсией логики не забудьте убрать инверсию логики
    Serial.println("Light: ON");
  } else {
    digitalWrite(flashlightPin, HIGH);
    Serial.println("Light: OFF");
  }
}