#include "encoder_menu.h"
#include "display.h" 
#include "display/FreeSerifItalic12pt7b.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

extern Adafruit_ST7789 tft;
extern void updateFastValues(); 
extern void drawModeAndAlarm();

// Colores de la interfaz
uint16_t colorUI_Text = 0xFFFF;    // Blanco
uint16_t colorUI_Active = 0x07E0;  // Verde
uint16_t colorUI_Inactive = 0xF800;// Rojo

volatile bool inMenu = false;
volatile bool ActivatedAlarm = false;
volatile bool modeWarningActive = false;
volatile int warningSelection = 0; // 0 = Cancelar, 1 = Cambiar

volatile int currentModeIndex = 0; 
volatile float currentTargetTemp = 20.0;
volatile float currentTargetHum = 60.0;

volatile bool systemRunning = false; 
volatile unsigned long remainingSeconds = 0;

const float targetTemps[5] = {20.0, 22.0, 25.0, 28.0, 45.0};
const float targetHums[5]  = {60.0, 65.0, 70.0, 75.0, 30.0};
const unsigned long targetTimes[5] = {14400, 7200, 10800, 14400, 43200}; 

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
unsigned long lastTimerTick = 0; 

void setupEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP); 
  lastClk = digitalRead(ENCODER_CLK);

  currentTargetTemp = targetTemps[currentModeIndex];
  currentTargetHum = targetHums[currentModeIndex];
  remainingSeconds = targetTimes[currentModeIndex]; 
}

void advanceMode() {
  currentModeIndex++;
  if (currentModeIndex > 4) currentModeIndex = 0;
  currentTargetTemp = targetTemps[currentModeIndex];
  currentTargetHum = targetHums[currentModeIndex];
  remainingSeconds = targetTimes[currentModeIndex];
}

void redrawFullDashboard() {
  drawStaticBase();
  drawModeAndAlarm();
  updateFastValues();
}

void drawWarningOverlay() {
  displayLock();

  tft.fillRect(20, 60, 200, 130, 0x2124); // Fondo gris oscuro
  tft.drawRect(20, 60, 200, 130, colorUI_Text); // Borde
  
  tft.setFont(nullptr);
  tft.setTextSize(1);
  tft.setTextColor(colorUI_Text);
  tft.setCursor(30, 75);
  tft.print("Si cambias de modo se");
  tft.setCursor(30, 90);
  tft.print("reiniciara la cuenta.");

  // Botón Cancelar (Rojo)
  tft.fillRect(35, 130, 70, 40, colorUI_Inactive);
  tft.setCursor(45, 145);
  tft.setTextColor(colorUI_Text);
  tft.print("Cancelar");

  // Botón Cambiar (Verde)
  tft.fillRect(135, 130, 70, 40, colorUI_Active);
  tft.setCursor(150, 145);
  tft.print("Cambiar");

  // Cuadro de selección (resaltador blanco)
  if (warningSelection == 0) {
    tft.drawRect(33, 128, 74, 44, colorUI_Text);
    tft.drawRect(133, 128, 74, 44, 0x2124); // Borrar el otro borde
  } else {
    tft.drawRect(33, 128, 74, 44, 0x2124);
    tft.drawRect(133, 128, 74, 44, colorUI_Text);
  }
  displayUnlock();
}

void drawMenuScreen() {
  displayLock();
  tft.fillScreen(0x0000); 
  tft.setFont(nullptr); 
  tft.setTextSize(2);
  tft.setTextColor(colorUI_Text); 
  tft.setCursor(20, 20);
  tft.println("--- MENU PRINCIPAL ---");

  tft.setFont(&FreeSerifItalic12pt7b);
  tft.setTextSize(1);
  for (int i = 0; i < NUM_OPTIONS; i++) {
    int yPos = 80 + (i * 40); 
    
    if (i == selectedOption) {
      tft.fillRect(10, yPos - 20, 300, 30, colorUI_Active); 
      tft.setTextColor(0x0000); 
      tft.setCursor(20, yPos);
      tft.print("> ");
    } else {
      tft.setTextColor(colorUI_Text); 
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
  displayUnlock();
}

void processEncoder() {
  // EL TEMPORIZADOR NUNCA SE DETIENE (incluso en menús)
  if (systemRunning) {
    if (millis() - lastTimerTick >= 1000) {
      lastTimerTick = millis();
      if (remainingSeconds > 0) {
        remainingSeconds--;
      } else {
        systemRunning = false; 
      }
    }
  }

  // --- NAVEGACIÓN OVERLAY DE ADVERTENCIA ---
  if (modeWarningActive) {
    int currentClk = digitalRead(ENCODER_CLK);
    if (currentClk != lastClk && currentClk == LOW) {
      warningSelection = !warningSelection; // Alterna 0 y 1
      drawWarningOverlay();
    }
    lastClk = currentClk;

    if (digitalRead(BTN_SELECT) == LOW) {
      if (millis() - lastSelectBtnTime > 300) {
        if (warningSelection == 0) {
          // Cancelar
          modeWarningActive = false;
          redrawFullDashboard();
        } else {
          // Cambiar
          modeWarningActive = false;
          systemRunning = false;
          advanceMode();
          redrawFullDashboard();
        }
        lastSelectBtnTime = millis();
      }
    }
    return; // Sale de la función, bloquea otras interacciones
  }

  // 1. Botón Encoder (Menú)
  if (digitalRead(ENCODER_SW) == LOW) {
    if (millis() - lastEncoderBtnTime > 300) { 
      if (!inMenu) {
        inMenu = true;
        selectedOption = 0;
        drawMenuScreen();
      } else {
        inMenu = false;
        redrawFullDashboard();
      }
      lastEncoderBtnTime = millis();
    }
  }

  // 2. Botón SELECT (En el menú)
  if (inMenu && digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastSelectBtnTime > 300) {
      if (selectedOption == 0) { 
        ActivatedAlarm = !ActivatedAlarm;
        inMenu = false;
        redrawFullDashboard();
      } 
      else if (selectedOption == 3) { 
        inMenu = false;
        redrawFullDashboard();
      } 
      else { 
        displayLock();
        tft.fillScreen(0x0000);
        tft.setFont(nullptr);
        tft.setCursor(20, 120);
        tft.setTextColor(colorUI_Text);
        tft.setTextSize(2);
        tft.print("Opcion en desarrollo");
        displayUnlock();

        delay(1500);
        drawMenuScreen(); 
      }
      lastSelectBtnTime = millis();
    }
  }

  // 3. Botón SELECT (Fuera del menú - Start/Stop)
  if (!inMenu && digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastSelectBtnTime > 300) {
      systemRunning = !systemRunning; 
      updateFastValues(); 
      lastSelectBtnTime = millis();
    }
  }

  // 4. Botón MODO
  if (!inMenu && digitalRead(BTN_MODE) == LOW) {
    if (millis() - lastModeBtnTime > 300) {
      if (systemRunning) {
        modeWarningActive = true;
        warningSelection = 0; // Por defecto "Cancelar"
        drawWarningOverlay();
      } else {
        advanceMode();
        drawModeAndAlarm(); // Actualiza solo Modo
        updateFastValues(); // Actualiza Metas y temporizador
      }
      lastModeBtnTime = millis();
    }
  }

  // 5. Rotación de menú
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