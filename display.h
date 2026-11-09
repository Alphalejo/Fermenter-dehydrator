#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

void initDisplayTask();
void displayTask(void *pvParameters);
void drawStaticBase(void);
void updateDynamicValues(void);