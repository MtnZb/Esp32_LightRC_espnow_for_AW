#ifndef DISPLAY_GAUGE_H
#define DISPLAY_GAUGE_H

#include <TFT_eSPI.h>

// Внешние объекты, чтобы файл видел tft и спрайт из основного кода
extern TFT_eSPI tft;
extern TFT_eSprite canvas;

// Параметры шкалы
extern float smoothNumber;
extern float targetNumber;
extern bool isOverheating;

const float easing = 0.08; 
const int myStart = 45;                
const int mySpan = 235;                

void drawGaugeUpdate() {
  // 1. Математика плавности
  float diff = targetNumber - smoothNumber;
  smoothNumber += diff * easing;

  // 2. Логика режима перегрева
  if (smoothNumber >= 99.0) isOverheating = true;
  if (isOverheating && smoothNumber <= 1.0) isOverheating = false;

  // 3. Выбор цвета (с градиентом)
  uint16_t color;
  if (isOverheating) {
    float pulse = (sin(millis() / 150.0) * 80.0) + 175.0; 
    color = tft.color565((int)pulse, 0, 0); 
  } else if (smoothNumber < 50.0) {
    color = TFT_YELLOW;
  } else {
    float ratio = (smoothNumber - 50.0) / 50.0;
    int g = 255 * (1.0 - ratio);
    color = tft.color565(255, g, 0);
  }

  // 4. Отрисовка в спрайте
  canvas.fillSprite(0x04FF); 

  int currentEnd = myStart + (int)(smoothNumber * (mySpan / 100.0));

  // Подложка
  canvas.drawArc(80, 80, 75, 35, myStart, myStart + mySpan, 0x2104, 0x04FF);
  // Активная часть
  canvas.drawArc(80, 80, 75, 35, myStart, currentEnd, color, 0x04FF);

  // Текст
  canvas.setTextColor(TFT_WHITE, 0x04FF);
  canvas.drawNumber((int)smoothNumber, 65, 75, 4);

  canvas.pushSprite(0, 0);
}

#endif