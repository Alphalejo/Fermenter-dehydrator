#include "encoder_menu.h"
#include "display.h" 
#include "display/FreeSerifItalic12pt7b.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

extern Adafruit_ST7789 tft;

volatile bool inMenu = false;
volatile bool ActivatedAlarm = false;

volatile int currentModeIndex = 0; 
volatile float currentTargetTemp = 20.0;
volatile float currentTargetHum = 60.0;

// Estado del sistema y temporizador
volatile bool systemRunning = false; 
volatile unsigned long remainingSeconds = 0;

// Configuración de los 5 modos: Temp, Hum, y Tiempo en segundos (Ej: 4h = 14400s)
const float targetTemps[5] = {20.0, 22.0, 25.0, 28.0, 45.0};
const float targetHums[5]  = {60.0, 65.0, 70.0, 75.0, 30.0};
const unsigned long targetTimes[5] = {14400, 7200, 10800, 14400, 43200}; // 4h, 2h, 3h, 4h, 12h

int selectedOption = 0;
const int NUM_OPTIONS = 4;

const char* menuOptions[] = {
  "1. Activar/Desactivar Alarma",
  "2. Personalizacion",
  "3. Mas Informacion",
  "4. Salir del Menu"
};

int lastClk = HIGH;
unsigned long lastEncoderBtnTime = 0;
unsigned long lastSelectBtnTime = 0;
unsigned long lastModeBtnTime = 0; 
unsigned long lastTimerTick = 0; // NUEVO: Para el contador regresivo

void setupEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP); 
  lastClk = digitalRead(ENCODER_CLK);

  currentTargetTemp = targetTemps[currentModeIndex];
  currentTargetHum = targetHums[currentModeIndex];
  remainingSeconds = targetTimes[currentModeIndex]; // Cargar tiempo inicial
}

void drawMenuScreen() {
  tft.fillScreen(0x0000); 
  tft.setFont(nullptr); 
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF); 
  tft.setCursor(20, 20);
  tft.println("--- MENU PRINCIPAL ---");

  tft.setFont(&FreeSerifItalic12pt7b);
  tft.setTextSize(1);
  for (int i = 0; i < NUM_OPTIONS; i++) {
    int yPos = 80 + (i * 40); 
    
    if (i == selectedOption) {
      tft.fillRect(10, yPos - 20, 300, 30, 0x07E0); 
      tft.setTextColor(0x0000); 
      tft.setCursor(20, yPos);
      tft.print("> ");
    } else {
      tft.setTextColor(0xFFFF); 
      tft.setCursor(20, yPos);
      tft.print("   ");
    }
    
    if (i == 0) {
      if (ActivatedAlarm) tft.print("1. Desactivar Alarma");
      else tft.print("1. Activar Alarma");
    } else {
      tft.print(menuOptions[i]);
    }
  }
}

void processEncoder() {
  // --- NUEVO: Lógica del contador regresivo ---
  if (systemRunning) {
    if (millis() - lastTimerTick >= 1000) {
      lastTimerTick = millis();
      if (remainingSeconds > 0) {
        remainingSeconds--;
      } else {
        systemRunning = false; // El tiempo terminó, apagar actuadores
      }
    }
  }

  // 1. Botón del Encoder (Abrir/Cerrar menú)
  if (digitalRead(ENCODER_SW) == LOW) {
    if (millis() - lastEncoderBtnTime > 300) { 
      if (!inMenu) {
        inMenu = true;
        selectedOption = 0;
        drawMenuScreen();
      } else {
        inMenu = false;
        drawStaticBase();      
        updateDynamicValues(); 
      }
      lastEncoderBtnTime = millis();
    }
  }

  // 2. Botón SELECT (DENTRO del menú: Elegir opción)
  if (inMenu && digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastSelectBtnTime > 300) {
      if (selectedOption == 0) { 
        ActivatedAlarm = !ActivatedAlarm;
        inMenu = false;
        drawStaticBase();      
        updateDynamicValues(); 
      } 
      else if (selectedOption == 3) { 
        inMenu = false;
        drawStaticBase();      
        updateDynamicValues(); 
      } 
      else { 
        tft.fillScreen(0x0000);
        tft.setCursor(20, 120);
        tft.setTextColor(0xFFFF);
        tft.setTextSize(2);
        tft.print("Opcion en desarrollo");
        delay(1500);
        drawMenuScreen(); 
      }
      lastSelectBtnTime = millis();
    }
  }

  // 3. Botón SELECT (FUERA del menú: INICIAR / PAUSAR el sistema)
  if (!inMenu && digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastSelectBtnTime > 300) {
      systemRunning = !systemRunning; // Alternar estado
      updateDynamicValues(); // Refrescar pantalla inmediatamente
      lastSelectBtnTime = millis();
    }
  }

  // 4. Botón MODO (Cambia de modo, detiene el sistema y reinicia el tiempo)
  if (!inMenu && digitalRead(BTN_MODE) == LOW) {
    if (millis() - lastModeBtnTime > 300) {
      currentModeIndex++;
      if (currentModeIndex > 4) currentModeIndex = 0;
      
      systemRunning = false; // Detener actuadores al cambiar de modo por seguridad
      currentTargetTemp = targetTemps[currentModeIndex];
      currentTargetHum = targetHums[currentModeIndex];
      remainingSeconds = targetTimes[currentModeIndex]; // Reiniciar el timer
      
      updateDynamicValues();
      lastModeBtnTime = millis();
    }
  }

  // 5. Lectura de rotación (Navegar menú)
  if (inMenu) {
    int currentClk = digitalRead(ENCODER_CLK);
    if (currentClk != lastClk && currentClk == LOW) {
      int dtValue = digitalRead(ENCODER_DT);
      if (dtValue == HIGH) { 
        selectedOption++;
        if (selectedOption >= NUM_OPTIONS) selectedOption = 0; 
      } else {               
        selectedOption--;
        if (selectedOption < 0) selectedOption = NUM_OPTIONS - 1; 
      }
      drawMenuScreen();
    }
    lastClk = currentClk;
  }
}