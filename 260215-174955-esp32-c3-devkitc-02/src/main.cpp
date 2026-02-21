#include <Arduino.h> // Обязательно для VS Code
#include <SPI.h>
#include <TFT_eSPI.h>
#include <esp_now.h>
#include <WiFi.h>
SPIClass hspi(HSPI);
TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
uint8_t broadcastAddress[] = {0x48, 0x3F, 0xDA, 0x5F, 0x4F, 0x77};
const int buttonPin = 3;
unsigned long previousMillis = 0;  // Таймер для счетчика
const long interval = 1000;
static bool lastButtonState = HIGH;
typedef struct struct_message {
    bool Light_On;
} struct_message;

struct_message EspMessage;
esp_now_peer_info_t peerInfo;


void setup() {
  Serial.begin(115200);
  // Initialize TFT display
  tft.init();

  pinMode(buttonPin, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    tft.drawString("ESP-NOW Error", 10, 10, 2);
    return;
  }

  // Регистрация получателя (пира)
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    tft.drawString("Peer Error", 10, 10, 2);
    return;
  }

  // Display color transitions
  tft.fillScreen(TFT_BLUE);
  delay(1000);
  tft.fillScreen(TFT_RED);
  delay(1000);
  tft.fillScreen(TFT_GREEN);
  delay(1000);

  // Set background color and font color
  tft.fillScreen(0x04FF);  // RGB565 color
  tft.setTextColor(TFT_WHITE, 0x04FF);  // Set text color to white with background

  // Display welcome message
  tft.drawString("Wake Up!", 40, 40, 2);
}

int number = 0;
bool overheated = false;
unsigned long lastUpdate = 0;

// НАСТРОЙКИ БАЛАНСА
const int updateSpeed = 30;   // Шаг обновления логики (мс)
const int coolDownSpeed = 3;  // Коэффициент охлаждения (чем больше, тем медленнее остывает)

int coolDownCounter = 0;      // Вспомогательный счетчик для замедления

void loop() {
  unsigned long currentMillis = millis();
  bool currentButtonState = digitalRead(buttonPin);
  String displayNumber = String(number);
  if (currentMillis - lastUpdate >= updateSpeed) {                
    lastUpdate = currentMillis;

    if (!overheated) {
      if (currentButtonState == LOW) {
        // НАГРЕВ: кнопка нажата, растем на каждом шаге
        if (number < 100) number++;
      } 
      else {
        // ОХЛАЖДЕНИЕ: используем делитель скорости
        if (number > 0) {
          coolDownCounter++;
          if (coolDownCounter >= coolDownSpeed) {
            number--;
            coolDownCounter = 0; // Сбрасываем счетчик замедления
          }
        }
      }
    } 
    else {
      // ПЕРЕГРЕВ: остываем чуть быстрее или так же, пока не дойдем до 0
      if (number > 0) {
        coolDownCounter++;
        if (coolDownCounter >= 2) { // В режиме перегрева остываем стабильно
           number--;
           coolDownCounter = 0;
        }
      } else {
        overheated = false; // Остыли полностью, блокировка снята
      }
    }

    if (number >= 100 && !overheated) {
      overheated = true;
      EspMessage.Light_On = false;
      for(int i = 0; i < 3; i++) {
        esp_now_send(broadcastAddress, (uint8_t *) &EspMessage, sizeof(EspMessage));
        delay(3); 
      }
    }

    char buf[10];
    sprintf(buf, "%02d%% ", number);
    
    if (overheated) {
      tft.setTextColor(TFT_RED, 0x04FF);
      tft.drawString("CRITICAL!", 35, 40, 2);
    } else {
      tft.setTextColor(TFT_WHITE, 0x04FF);
      tft.drawString("Temp:    ", 40, 40, 2); 
    }
    tft.drawString(buf, 50, 80, 6);
  }
  // ОТПРАВКА КОМАНДЫ (если нет перегрева)
  if (currentButtonState != lastButtonState) {
    delay(20); 
    if (!overheated) {
      EspMessage.Light_On = (currentButtonState == LOW);
      for(int i = 0; i < 3; i++) {
      esp_now_send(broadcastAddress, (uint8_t *) &EspMessage, sizeof(EspMessage));
      delay(5); 
      }
    }
    lastButtonState = currentButtonState;
  }
}