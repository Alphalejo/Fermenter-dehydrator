#include "display.h"
#include "display/FreeSans9pt7b.h"
#include "display/FreeSerifBold9pt7b.h"
#include "display/FreeSerifItalic12pt7b.h"
#include "display/icons/icons.h"
#include "sensors.h" 
#include "encoder_menu.h"

#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  17
#define TFT_BL   4


extern volatile float currentTemperature;
extern volatile float currentHumidity;

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

int counter = 0;

void drawStaticBase(void) {
    tft.fillScreen(0x0);

    // static labels
    tft.setTextColor(0xFFFF);
    tft.setTextWrap(false);
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setCursor(46, 126);
    tft.print("Temp");

    tft.setTextSize(1);
    tft.setCursor(187, 127);
    tft.print("Hum");

    tft.setFont(&FreeSerifItalic12pt7b);
    tft.setCursor(232, 33);
    tft.print("Modo 2");

    tft.setCursor(140, 31);
    tft.print("E1");

    tft.setFont(&FreeSerifBold9pt7b);
    tft.setCursor(102, 126);
    tft.print("25");

    tft.setTextSize(1);
    tft.setCursor(127, 126);
    tft.print("C");

    tft.setTextSize(2);
    tft.setCursor(114, 102);
    tft.print("C");

    tft.setTextSize(1);
    tft.setCursor(237, 127);
    tft.print("70%");

    // static shapes and icons
    tft.drawCircle(123, 117, 2, 0xFFFF);
    tft.drawCircle(113, 81, 3, 0xFFFF);
    tft.drawBitmap(185, 75, image_weather_humidity_bits, 19, 31, 0xFFFF);
    tft.drawBitmap(49, 75, image_weather_temperature_bits, 18, 32, 0xFFFF);
    tft.drawRGBBitmap(34, 177, image_paint_17_pixels, 45, 46);
    tft.drawRGBBitmap(199, 14, image_paint_19_1_pixels, 25, 23);
    tft.drawLine(11, 46, 305, 46, 0xFFFF);
    tft.drawLine(10, 154, 304, 154, 0xFFFF);
}

void updateDynamicValues(void) {
    // Temperature (clear previous value first)
    tft.fillRect(72, 82, 40, 20, 0x0);
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF);
    tft.setCursor(72, 101);
    tft.print((int)currentTemperature);

    // Humidity (clear previous value first)
    tft.fillRect(208, 78, 40, 30, 0x0);
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setTextSize(2);
    tft.setCursor(208, 100);
    tft.print((int)currentHumidity);
    tft.print("%");

    // Clock (clear previous value first)
    tft.fillRect(86, 195, 160, 25, 0x0);
    tft.setFont(nullptr);
    tft.setTextSize(3);
    tft.setFont(&FreeSans9pt7b);
    tft.setCursor(86, 215);
    tft.print("07:25:32"); // replace with real time variable when the RTC is added
}

void displayTask(void *pvParameters) {
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  drawStaticBase(); // draw everything static ONCE

  unsigned long lastValuesUpdate = 0;
  unsigned long lastIconUpdate = 0;

  for (;;) {
  unsigned long now = millis();

  // ONLY update the dashboard if we are NOT in the menu
  if (!inMenu) {
    // Dynamic values: temperature, humidity, clock — refresh at 1 Hz
    if (now - lastValuesUpdate >= 1000) {
      updateDynamicValues();
      lastValuesUpdate = now;
    }

    // Animated icons: refresh independently at 8 Hz
    if (now - lastIconUpdate >= 125) {
      runIcon(flameIcon, tft);
      runIcon(fanIcon, tft);
      runIcon(humidifierIcon, tft);
      lastIconUpdate = now;
    }
  }

  vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void initDisplayTask() {
  xTaskCreate(
    displayTask,
    "DisplayTask",
    4096,
    NULL,
    1,
    NULL
  );
}