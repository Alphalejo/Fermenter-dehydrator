#ifndef ENCODER_MENU_H
#define ENCODER_MENU_H

#include <Arduino.h>

#define ENCODER_CLK 14
#define ENCODER_DT  15
#define ENCODER_SW  13
#define BTN_SELECT  26
#define BTN_MODE    25  

extern volatile bool inMenu;
extern volatile bool ActivatedAlarm;

// Variables de los modos
extern volatile int currentModeIndex; 
extern volatile float currentTargetTemp;
extern volatile float currentTargetHum;

// NUEVO: Variables para el temporizador y estado del sistema
extern volatile bool systemRunning; 
extern volatile unsigned long remainingSeconds;

void setupEncoder();
void drawMenuScreen();
void processEncoder();

#endif