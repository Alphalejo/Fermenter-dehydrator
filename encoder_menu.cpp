#include "encoder_menu.h"
#include "display.h" 
#include "display/FreeSerifItalic12pt7b.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>

Preferences preferences;
extern Adafruit_ST7789 tft;

extern void updateFastValues(); 
extern void drawModeAndAlarm();

// Variables de sensores para la pantalla de Información
extern volatile float sht1_temp;
extern volatile float sht1_hum;
extern volatile float sht2_temp;
extern volatile float sht2_hum;
extern volatile float humidityEntrance;
extern volatile float humidityOutput;
extern volatile float tempEntrance;
extern volatile float tempOutput;
extern volatile bool sensorMismatchError;

// Colores de la interfaz
uint16_t colorUI_Text = 0xFFFF;    
uint16_t colorUI_Active = 0x07E0;  
uint16_t colorUI_Inactive = 0xF800;

volatile bool inMenu = false;
volatile bool inInfoScreen = false;
volatile bool ActivatedAlarm = false;
volatile bool modeWarningActive = false;
volatile int warningSelection = 0; 

// Variables del Personalizador
volatile int configState = 0; 
int configOption = 0;
int editModeIndex = 0;
bool isEditingVal = false; 

volatile int currentModeIndex = 0; 
volatile float currentTargetTemp = 20.0;
volatile float currentTargetHum = 60.0;
volatile bool systemRunning = false; 
volatile unsigned long remainingSeconds = 0;

// ARREGLOS (Ahora sin 'const' para poder editarlos y guardarlos)
float targetTemps[5] = {20.0, 22.0, 25.0, 28.0, 45.0};
float targetHums[5]  = {60.0, 65.0, 70.0, 75.0, 30.0};
unsigned long targetTimes[5] = {14400, 7200, 10800, 14400, 43200}; 

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

void loadSettings();
void saveSettings();
void drawConfigMenu();
void drawConfigModes();
void drawConfigColors();
void drawInfoScreen();

// --- FUNCION DEL FOOTER DINAMICO ---
void drawFooter(const char* selText, const char* menuText, const char* modeText) {
  displayLock();
  tft.fillRect(0, 225, 320, 15, 0x2124); // Fondo gris oscuro
  tft.setFont(nullptr);
  tft.setTextSize(1);
  tft.setTextColor(colorUI_Text);
  tft.setCursor(5, 228);   tft.print(selText);
  tft.setCursor(110, 228); tft.print(menuText);
  tft.setCursor(215, 228); tft.print(modeText);
  displayUnlock();
}

void loadSettings() {
  preferences.begin("settings", true); 
  colorUI_Active = preferences.getUShort("c_act", 0x07E0);
  colorUI_Inactive = preferences.getUShort("c_inact", 0xF800);
  if (preferences.getBytesLength("t_temps") > 0) preferences.getBytes("t_temps", targetTemps, sizeof(targetTemps));
  if (preferences.getBytesLength("t_hums") > 0) preferences.getBytes("t_hums", targetHums, sizeof(targetHums));
  if (preferences.getBytesLength("t_time") > 0) preferences.getBytes("t_time", targetTimes, sizeof(targetTimes));
  preferences.end();
}

void saveSettings() {
  preferences.begin("settings", false); 
  preferences.putUShort("c_act", colorUI_Active);
  preferences.putUShort("c_inact", colorUI_Inactive);
  preferences.putBytes("t_temps", targetTemps, sizeof(targetTemps));
  preferences.putBytes("t_hums", targetHums, sizeof(targetHums));
  preferences.putBytes("t_time", targetTimes, sizeof(targetTimes));
  preferences.end();
}

void setupEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP); 
  lastClk = digitalRead(ENCODER_CLK);
  loadSettings(); 
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
  drawStaticBase(); // Esto pintará automáticamente el footer del main dashboard
  drawModeAndAlarm();
  updateFastValues();
}

void drawWarningOverlay() {
  displayLock();
  tft.fillRect(20, 60, 280, 130, 0x2124); 
  tft.drawRect(20, 60, 280, 130, colorUI_Text); 
  tft.setFont(nullptr);
  tft.setTextSize(2);
  tft.setTextColor(colorUI_Text);
  tft.setCursor(30, 75); tft.print("Si cambias de modo se");
  tft.setCursor(30, 90); tft.print("reiniciara la cuenta.");

  tft.fillRect(35, 130, 115, 40, colorUI_Inactive);
  tft.setCursor(45, 145); tft.setTextColor(colorUI_Text); tft.print("Cancelar");

  tft.fillRect(165, 130, 115, 40, colorUI_Active);
  tft.setCursor(175, 145); tft.print("Cambiar");

  if (warningSelection == 0) {
    tft.drawRect(33, 128, 119, 44, colorUI_Text);
    tft.drawRect(163, 128, 119, 44, 0x2124); 
  } else {
    tft.drawRect(33, 128, 119, 44, 0x2124);
    tft.drawRect(163, 128, 119, 44, colorUI_Text);
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
    } else tft.print(menuOptions[i]);
  }
  displayUnlock();
  drawFooter("SEL: Aceptar", "MENU: Atras", "MODO: Guardar");
}

void drawInfoScreen() {
  displayLock(); 
  tft.fillScreen(0x0000); 
  tft.setFont(nullptr); 
  tft.setTextSize(2);
  tft.setTextColor(colorUI_Text);
  tft.setCursor(20, 15);
  tft.println("--- MAS INFORMACION ---");

  tft.setTextSize(1);
  tft.setCursor(15, 50);  tft.printf("SHT1 (T1/H1): %.1f C | %.1f %%\n", sht1_temp, sht1_hum);
  tft.setCursor(15, 75);  tft.printf("SHT2 (T2/H2): %.1f C | %.1f %%\n", sht2_temp, sht2_hum);
  tft.setCursor(15, 100); tft.printf("AHT10 (Ent): %.1f C | %.1f %%\n", tempEntrance, humidityEntrance);
  tft.setCursor(15, 125); tft.printf("AHT10 (Sal): %.1f C | %.1f %%\n", tempOutput, humidityOutput);

  tft.setCursor(15, 160); tft.print("Errores:");
  tft.setCursor(15, 175);
  if (sensorMismatchError) {
    tft.setTextColor(colorUI_Inactive); tft.print("E1: ALERTA DE REDUNDANCIA SHT31");
  } else {
    tft.setTextColor(colorUI_Active); tft.print("E1: OK (Sin errores)");
  }
  displayUnlock();
  drawFooter("SEL: ---", "MENU: Atras", "MODO: Guardar");
}

void drawConfigMenu() {
  displayLock();
  tft.fillScreen(0x0000);
  tft.setFont(nullptr);
  tft.setTextSize(2);
  tft.setTextColor(colorUI_Text);
  tft.setCursor(15, 15);
  tft.println("- PERSONALIZACION -");
  
  tft.setFont(&FreeSerifItalic12pt7b);
  tft.setTextSize(1);
  const char* opts[] = {"1. Metas de Modos", "2. Paleta Colores", "3. Volver"};
  for(int i = 0; i < 3; i++) {
    int yPos = 80 + (i * 45);
    if(i == configOption) {
      tft.fillRect(10, yPos - 20, 300, 30, colorUI_Active);
      tft.setTextColor(0x0000);
      tft.setCursor(20, yPos); tft.print("> ");
    } else {
      tft.setTextColor(colorUI_Text);
      tft.setCursor(20, yPos); tft.print("   ");
    }
    tft.print(opts[i]);
  }
  displayUnlock();
  drawFooter("SEL: Aceptar", "MENU: Atras", "MODO: Guardar");
}

void drawConfigModes() {
  displayLock();
  tft.fillScreen(0x0000);
  tft.setFont(nullptr);
  tft.setTextSize(2);
  tft.setTextColor(colorUI_Text);
  tft.setCursor(25, 10);
  tft.println("--- EDITAR METAS ---");
  
  tft.setFont(&FreeSerifItalic12pt7b);
  tft.setTextSize(1);
  const char* modeN[] = {"Modo 1", "Modo 2", "Modo 3", "Modo 4", "Secado"};
  
  for(int i = 0; i < 5; i++) {
    int yPos = 55 + (i * 38); // Espaciado para que quepan 5 opciones
    
    if(i == configOption) {
      tft.fillRect(10, yPos - 18, 300, 26, isEditingVal ? 0xFFE0 : colorUI_Active); 
      tft.setTextColor(0x0000);
    } else tft.setTextColor(colorUI_Text);
    
    tft.setCursor(20, yPos);
    if (i == 0) tft.printf("%s %s", (i==configOption)?"> ":"  ", modeN[editModeIndex]);
    else if (i == 1) tft.printf("%s Temp: %.0f C", (i==configOption)?"> ":"  ", targetTemps[editModeIndex]); // Formato de 1 en 1
    else if (i == 2) tft.printf("%s Hum: %.0f %%", (i==configOption)?"> ":"  ", targetHums[editModeIndex]);
    else if (i == 3) {
      // Mostrar el tiempo en Horas y Minutos
      int h = targetTimes[editModeIndex] / 3600;
      int m = (targetTimes[editModeIndex] % 3600) / 60;
      tft.printf("%s Tiempo: %dh %02dm", (i==configOption)?"> ":"  ", h, m);
    }
    else if (i == 4) tft.printf("%s Volver", (i==configOption)?"> ":"  ");
  }
  displayUnlock();
  drawFooter(isEditingVal ? "SEL: Aplicar" : "SEL: Editar", "MENU: Atras", "MODO: Guardar");
}

void drawConfigColors() {
  displayLock();
  tft.fillScreen(0x0000);
  tft.setFont(nullptr);
  tft.setTextSize(2);
  tft.setTextColor(colorUI_Text);
  tft.setCursor(40, 15);
  tft.println("--- COLORES ---");
  
  tft.setFont(&FreeSerifItalic12pt7b);
  tft.setTextSize(1);
  const char* opts[] = {"1. Clasico (Verde/Rojo)", "2. Cyber (Cian/Magen)", "3. Solar (Ama/Naran)", "4. Volver"};
  for(int i = 0; i < 4; i++) {
    int yPos = 75 + (i * 40);
    if(i == configOption) {
      tft.fillRect(10, yPos - 20, 300, 30, colorUI_Active);
      tft.setTextColor(0x0000);
      tft.setCursor(20, yPos); tft.print("> ");
    } else {
      tft.setTextColor(colorUI_Text);
      tft.setCursor(20, yPos); tft.print("   ");
    }
    tft.print(opts[i]);
  }
  displayUnlock();
  drawFooter("SEL: Aplicar", "MENU: Atras", "MODO: Guardar");
}

void processEncoder() {
  // 1. Temporizador siempre corriendo atrás
  if (systemRunning) {
    if (millis() - lastTimerTick >= 1000) {
      lastTimerTick = millis();
      if (remainingSeconds > 0) remainingSeconds--;
      else systemRunning = false; 
    }
  }

  // 2. Leer estado limpio y con Debounce de todos los botones
  bool btnMenu   = false;
  bool btnSelect = false;
  bool btnMode   = false;

  if (digitalRead(ENCODER_SW) == LOW) {
    if (millis() - lastEncoderBtnTime > 300) { btnMenu = true; lastEncoderBtnTime = millis(); }
  }
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastSelectBtnTime > 300) { btnSelect = true; lastSelectBtnTime = millis(); }
  }
  if (digitalRead(BTN_MODE) == LOW) {
    if (millis() - lastModeBtnTime > 300) { btnMode = true; lastModeBtnTime = millis(); }
  }

  int currentClk = digitalRead(ENCODER_CLK);
  bool rotated = (currentClk != lastClk && currentClk == LOW);
  int dir = (digitalRead(ENCODER_DT) == HIGH) ? 1 : -1;
  lastClk = currentClk; // Actualizar reloj inmediatamente

  // --- OVERLAY DE ADVERTENCIA ---
  if (modeWarningActive) {
    if (rotated) {
      warningSelection = !warningSelection;
      drawWarningOverlay();
    }
    if (btnSelect) {
      modeWarningActive = false;
      if (warningSelection == 1) { // Cambiar
        systemRunning = false;
        advanceMode();
      }
      redrawFullDashboard();
    }
    return; // Si estamos aquí, bloqueamos otras pantallas
  }

  // --- PANTALLA DE INFORMACION (Solo salir) ---
  if (inInfoScreen) {
    if (btnMenu) { inInfoScreen = false; drawMenuScreen(); }
    if (btnMode) { inInfoScreen = false; inMenu = false; redrawFullDashboard(); }
    return; 
  }

  // --- PERSONALIZACION ---
  if (configState > 0) {
    // BOTÓN MODO: Guardar y salir al Main Dashboard en cualquier momento
    if (btnMode) {
      saveSettings();
      configState = 0;
      inMenu = false;
      redrawFullDashboard();
      return;
    }

    if (rotated) {
      if (configState == 1) { // Menu principal config
        configOption += dir;
        if (configOption > 2) configOption = 0;
        if (configOption < 0) configOption = 2;
        drawConfigMenu();
      } 
      else if (configState == 2) { // Modos (Ahora con Tiempo)
        if (!isEditingVal) {
          configOption += dir;
          if (configOption > 4) configOption = 0;
          if (configOption < 0) configOption = 4;
        } else {
          if (configOption == 0) {
            editModeIndex += dir;
            if (editModeIndex > 4) editModeIndex = 0;
            if (editModeIndex < 0) editModeIndex = 4;
          } else if (configOption == 1) {
            targetTemps[editModeIndex] += dir * 1.0; // Sube de 1 en 1
          } else if (configOption == 2) {
            targetHums[editModeIndex] += dir * 1.0;  // Sube de 1 en 1
          } else if (configOption == 3) {
            // Aumenta o resta el tiempo en saltos de 30 minutos (1800 segundos)
            if (dir > 0) targetTimes[editModeIndex] += 1800;
            else if (targetTimes[editModeIndex] >= 1800) targetTimes[editModeIndex] -= 1800;
          }
          
          // Actualización instantánea en vivo
          if (editModeIndex == currentModeIndex) {
              currentTargetTemp = targetTemps[currentModeIndex];
              currentTargetHum = targetHums[currentModeIndex];
              remainingSeconds = targetTimes[currentModeIndex];
          }
        }
        drawConfigModes();
      }
      else if (configState == 3) { // Colores
        configOption += dir;
        if (configOption > 3) configOption = 0;
        if (configOption < 0) configOption = 3;
        drawConfigColors();
      }
    }

    // MENU BTN = Funciona estrictamente como un botón "Atrás"
    if (btnMenu) {
      if (configState == 1) { configState = 0; drawMenuScreen(); } 
      else if (configState == 2) {
        if (isEditingVal) isEditingVal = false;
        else { configState = 1; configOption = 0; drawConfigMenu(); }
        drawConfigModes();
      } 
      else if (configState == 3) { configState = 1; configOption = 1; drawConfigMenu(); }
    }

    // SELECT BTN = Funciona estrictamente como un botón "Enter/Aplicar"
    if (btnSelect) {
      if (configState == 1) {
        if (configOption == 0) { configState = 2; configOption = 0; isEditingVal = false; drawConfigModes(); }
        else if (configOption == 1) { configState = 3; configOption = 0; drawConfigColors(); }
        else if (configOption == 2) { saveSettings(); configState = 0; drawMenuScreen(); }
      } 
      else if (configState == 2) {
        if (configOption == 4) { configState = 1; configOption = 0; drawConfigMenu(); }
        else { isEditingVal = !isEditingVal; drawConfigModes(); }
      } 
      else if (configState == 3) {
        if (configOption == 0) { colorUI_Active = 0x07E0; colorUI_Inactive = 0xF800; } 
        else if (configOption == 1) { colorUI_Active = 0x07FF; colorUI_Inactive = 0xF81F; } 
        else if (configOption == 2) { colorUI_Active = 0xFFE0; colorUI_Inactive = 0xFC00; } 
        
        if (configOption == 3) { saveSettings(); configState = 1; configOption = 1; drawConfigMenu(); }
        else drawConfigColors(); 
      }
    }
    return; // Fin del bloque Personalización
  }

  // --- MENU PRINCIPAL ---
  if (inMenu) {
    if (btnMode) { // Guardar y Salir
      saveSettings();
      inMenu = false;
      redrawFullDashboard();
      return;
    }

    if (rotated) {
      selectedOption += dir;
      if (selectedOption >= NUM_OPTIONS) selectedOption = 0; 
      if (selectedOption < 0) selectedOption = NUM_OPTIONS - 1; 
      drawMenuScreen();
    }

    if (btnMenu) { // Atrás (Cerrar menú)
      inMenu = false;
      redrawFullDashboard();
    }

    if (btnSelect) { // Entrar
      if (selectedOption == 0) { ActivatedAlarm = !ActivatedAlarm; drawMenuScreen(); } 
      else if (selectedOption == 1) { configState = 1; configOption = 0; drawConfigMenu(); } 
      else if (selectedOption == 2) { inInfoScreen = true; drawInfoScreen(); } 
      else if (selectedOption == 3) { inMenu = false; redrawFullDashboard(); } 
    }
    return;
  }

  // --- DASHBOARD PRINCIPAL ---
  if (!inMenu && !inInfoScreen && configState == 0 && !modeWarningActive) {
    if (btnMenu) {
      inMenu = true;
      selectedOption = 0;
      drawMenuScreen();
    }
    if (btnSelect) {
      systemRunning = !systemRunning; 
      updateFastValues(); 
    }
    if (btnMode) {
      if (systemRunning) {
        modeWarningActive = true;
        warningSelection = 0; 
        drawWarningOverlay();
      } else {
        advanceMode();
        drawModeAndAlarm(); 
        updateFastValues(); 
      }
    }
  }
}