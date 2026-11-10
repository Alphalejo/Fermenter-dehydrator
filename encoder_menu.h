#ifndef ENCODER_MENU_H
#define ENCODER_MENU_H

#include <Arduino.h>

#define ENCODER_CLK 14
#define ENCODER_DT  15
#define ENCODER_SW  13
#define BTN_SELECT  26
#define BTN_MODE    25  

// Variables globales de la interfaz (Personalización futura)
extern uint16_t colorUI_Text;
extern uint16_t colorUI_Active;
extern uint16_t colorUI_Inactive;

extern volatile bool inMenu;
extern volatile bool ActivatedAlarm;

extern volatile int currentModeIndex; 
extern volatile float currentTargetTemp;
extern volatile float currentTargetHum;

extern volatile bool systemRunning; 
extern volatile unsigned long remainingSeconds;

// Variables para el menú de seguridad (Overlay)
extern volatile bool modeWarningActive;
extern volatile int warningSelection; // 0 = Cancelar, 1 = Cambiar

void setupEncoder();
void drawMenuScreen();
void drawWarningOverlay(); // Nueva función de dibujo
void redrawFullDashboard(); // Helper para refrescar todo
void processEncoder();

#endif