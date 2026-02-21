#include <SPI.h>
#include <TFT_eSPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include "display_gauge.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft);

float smoothNumber = 0;
float targetNumber = 0;
bool isOverheating = false;
bool isTesting = false;
String gaugeStatus = "";

uint8_t broadcastAddress[] = { 0x48, 0x3F, 0xDA, 0x5F, 0x4F, 0x77 };
const int buttonPin = 3;

typedef struct struct_message {
  bool Light_On;
} struct_message;

struct_message EspMessage;
esp_now_peer_info_t peerInfo;

unsigned long lastUpdate = 0;
const int updateSpeed = 30;  // Скорость обновления экрана и логики (33 FPS)
const int coolDownSpeed = 3;
int coolDownCounter = 0;
static bool lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(0);

  // Создаем спрайт для плавной отрисовки
  canvas.createSprite(160, 160);

  pinMode(buttonPin, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) return;

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) return;

  tft.fillScreen(0x04FF);
  tft.setTextColor(TFT_WHITE, 0x04FF);

  //Тестовый прогон
  isTesting = true;
  // 1. Имитация нагрева до максимума
  targetNumber = 100;
  gaugeStatus = "SYSTEM CHECK";
  while (smoothNumber < 99.0) {
    drawGaugeUpdate();  // Используем функцию из display_gauge.h
    delay(30);                // Совпадаем с updateSpeed [cite: 12]
  }

  // 2. Демонстрация пульсации перегрева (пауза 2 секунды)
  // В этот момент targetNumber уже 100, и флаг isOverheating станет true внутри drawGaugeUpdate
  gaugeStatus = "OVERHEATED";
  unsigned long testTimer = millis();
  while (millis() - testTimer < 2000) {
    drawGaugeUpdate();
    delay(30);
  }

  // 3. Имитация автоматического остывания
  targetNumber = 0;
  gaugeStatus = "COOLING DOWN";
  while (isOverheating) {  // Ждем, пока флаг перегрева не сбросится в display_gauge.h
    drawGaugeUpdate();
    delay(30);
  }
  isTesting = false;
  gaugeStatus = "";
  tft.fillScreen(0x04FF);
}

void loop() {
  unsigned long currentMillis = millis();
  bool currentButtonState = digitalRead(buttonPin);

  // --- 1. ОБНОВЛЕНИЕ ЛОГИКИ И ЭКРАНА (ПО ТАЙМЕРУ) ---
  if (currentMillis - lastUpdate >= updateSpeed) {
    lastUpdate = currentMillis;

    // Проверка на достижение критической температуры
    if (targetNumber >= 100 && !isOverheating) {
      isOverheating = true;
      // ПРИНУДИТЕЛЬНО ВЫКЛЮЧАЕМ ФОНАРЬ ПРИ ПЕРЕГРЕВЕ
      EspMessage.Light_On = false;
      for (int i = 0; i < 3; i++) {
        esp_now_send(broadcastAddress, (uint8_t *)&EspMessage, sizeof(EspMessage));
        delay(5);
      }
      Serial.println("CRITICAL: Overheat protection active!");
    }

    // Расчёт изменения targetNumber
    if (!isOverheating) {
      if (currentButtonState == LOW) {
        if (targetNumber < 100) targetNumber += 1.0;
      } else {
        if (targetNumber > 0) {
          coolDownCounter++;
          if (coolDownCounter >= coolDownSpeed) {
            targetNumber--;
            coolDownCounter = 0;
          }
        }
      }
    } else {
      // Режим остывания при перегреве
      if (targetNumber > 0) {
        coolDownCounter++;
        if (coolDownCounter >= 2) {
          targetNumber--;
          coolDownCounter = 0;
        }
      }
      // Флаг isOverheating сбросится сам внутри drawGaugeUpdate, когда smoothNumber < 1.0
    }

    // Отрисовка кадра (вызывается строго по таймеру)
    drawGaugeUpdate();
  }

  // --- 2. ОТПРАВКА КОМАНДЫ ПРИ НАЖАТИИ (СОБЫТИЙНАЯ) ---
  if (currentButtonState != lastButtonState) {
    delay(20);  // Антидребезг

    // Посылаем сигнал только если нет блокировки перегрева
    if (!isOverheating) {
      EspMessage.Light_On = (currentButtonState == LOW);
      for (int i = 0; i < 3; i++) {
        esp_now_send(broadcastAddress, (uint8_t *)&EspMessage, sizeof(EspMessage));
        delay(5);
      }
    } else {
      // Если перегрев активен, на любое нажатие шлём "выключено"
      EspMessage.Light_On = false;
      for (int i = 0; i < 2; i++) {
        esp_now_send(broadcastAddress, (uint8_t *)&EspMessage, sizeof(EspMessage));
        delay(5);
      }
    }
    lastButtonState = currentButtonState;
  }
}