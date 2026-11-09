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
extern volatile bool ActivatedAlarm;

// Traemos las variables dinámicas del modo desde el encoder
extern volatile int currentModeIndex;
extern volatile float currentTargetTemp;
extern volatile float currentTargetHum;

// NUEVO: Traemos el estado del sistema y temporizador
extern volatile bool systemRunning; 
extern volatile unsigned long remainingSeconds;

// Nombres para pintar en el header
const char* modeNames[5] = {"Modo 1", "Modo 2", "Modo 3", "Modo 4", "Secado"};

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void drawStaticBase(void) {
    tft.fillScreen(0x0);
    tft.setTextSize(1); // Reset de tamaño para evitar desbordes

    // Textos y etiquetas estáticas que NUNCA cambian
    tft.setTextColor(0xFFFF);
    tft.setTextWrap(false);
    
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setCursor(46, 126);
    tft.print("Temp");

    tft.setTextSize(1);
    tft.setCursor(187, 127);
    tft.print("Hum");

    tft.setFont(&FreeSerifItalic12pt7b);
    tft.setCursor(140, 31);
    tft.print("E1");

    // Mantener la "C" estática de temperatura objetivo
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setTextSize(1);
    tft.setCursor(127, 126);
    tft.print("C");

    // Mantener la "C" estática de temperatura principal
    tft.setTextSize(2);
    tft.setCursor(114, 102);
    tft.print("C");

    // Formas estáticas e iconos (Excepto el de la alarma, que es dinámico)
    tft.drawCircle(123, 117, 2, 0xFFFF);
    tft.drawCircle(113, 81, 3, 0xFFFF);
    tft.drawBitmap(185, 75, image_weather_humidity_bits, 19, 31, 0xFFFF);
    tft.drawBitmap(49, 75, image_weather_temperature_bits, 18, 32, 0xFFFF);
    tft.drawRGBBitmap(34, 177, clock_icon, 45, 46);
    tft.drawLine(11, 46, 305, 46, 0xFFFF);
    tft.drawLine(10, 154, 304, 154, 0xFFFF);
}

void updateDynamicValues(void) {
    // 1. Textos dinámicos del Modo (Header)
    tft.fillRect(190, 12, 110, 28, 0x0000); // Caja negra para borrar modo anterior
    tft.setFont(&FreeSerifItalic12pt7b);
    tft.setTextSize(1);
    tft.setTextColor(0xFFFF);
    
    // Ajuste posicional dinámico para centrar la palabra
    if (currentModeIndex == 4) tft.setCursor(225, 33); // "Secado" es más corto
    else tft.setCursor(232, 33); 
    tft.print(modeNames[currentModeIndex]);

    // 2. Metas objetivo dinámicas (Temp y Hum en el medio)
    tft.fillRect(98, 110, 27, 20, 0x0000); // Borra Target Temp
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setTextSize(1);
    tft.setTextColor(0xFFFF);
    tft.setCursor(102, 126);
    tft.print((int)currentTargetTemp);

    tft.fillRect(235, 112, 45, 20, 0x0000); // Borra Target Hum
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setTextSize(1);
    tft.setCursor(237, 126);
    tft.print((int)currentTargetHum);
    tft.print("%");

    // 3. Variables de sensores actuales SHT31 (Lecturas reales)
    tft.fillRect(72, 82, 40, 20, 0x0000);
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setTextSize(2);
    tft.setCursor(72, 101);
    tft.print((int)currentTemperature);

    tft.fillRect(208, 78, 40, 30, 0x0000);
    tft.setFont(&FreeSerifBold9pt7b);
    tft.setTextSize(2);
    tft.setCursor(208, 100);
    tft.print((int)currentHumidity);
    tft.print("%");

    // 4. Temporizador y Estado del Sistema (Inferior)
    tft.fillRect(60, 195, 200, 30, 0x0000); // Limpia área del reloj
    tft.setFont(nullptr);
    tft.setTextSize(3);
    
    // Cambiar color: Verde si está funcionando, Rojo si está en pausa
    if (systemRunning) {
        tft.setTextColor(0x07E0); // Verde
    } else {
        tft.setTextColor(0xF800); // Rojo
    }

    tft.setCursor(65, 200);

    // Calcular horas, minutos y segundos a partir de remainingSeconds
    int h = remainingSeconds / 3600;
    int m = (remainingSeconds % 3600) / 60;
    int s = remainingSeconds % 60;

    // Formatear string a "04:00:00"
    char timeStr[10];
    sprintf(timeStr, "%02d:%02d:%02d", h, m, s);
    tft.print(timeStr);

    // 5. Ícono de Alarma (Header derecho)
    if (ActivatedAlarm) {
        tft.drawRGBBitmap(199, 14, alarm_icon, 25, 23);
    } else {
        tft.fillRect(199, 14, 25, 23, 0x0000); // Borra ícono si está inactiva
    }
}

void displayTask(void *pvParameters) {
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  drawStaticBase(); 
  updateDynamicValues(); // <-- Llamada forzada al iniciar para que no quede la pantalla vacía

  unsigned long lastValuesUpdate = 0;
  unsigned long lastIconUpdate = 0;

  for (;;) {
    unsigned long now = millis();

    // SOLO actualiza el dashboard si NO estamos dentro del menú
    if (!inMenu) {
      // Valores dinámicos refrescados a 1 Hz
      if (now - lastValuesUpdate >= 1000) {
        updateDynamicValues();
        lastValuesUpdate = now;
      }

      // Iconos animados refrescados a 8 Hz
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