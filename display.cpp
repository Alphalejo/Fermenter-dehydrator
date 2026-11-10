#include "display.h"
#include "display/FreeSans9pt7b.h"
#include "display/FreeSerifBold9pt7b.h"
#include "display/FreeSerifItalic12pt7b.h"
#include "display/icons/icons.h"
#include "sensors.h"
#include "encoder_menu.h"
#include <freertos/semphr.h>
#include <math.h>
#include <string.h>

#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  17
#define TFT_BL   4

#define SCREEN_W 320
#define SCREEN_H 240
#define COLOR_BG 0x0000

extern volatile float currentTemperature;
extern volatile float currentHumidity;

const char* modeNames[5] = {"Modo 1", "Modo 2", "Modo 3", "Modo 4", "Secado"};

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ---------------------------------------------------------------------------
// TFT access lock.
// Every task that touches `tft` (display task, menu, alarm, ...) must wrap its
// drawing code in displayLock() / displayUnlock(). Declare both in display.h.
// ---------------------------------------------------------------------------
static SemaphoreHandle_t displayMutex = nullptr;

void displayLock() {
    if (displayMutex) xSemaphoreTakeRecursive(displayMutex, portMAX_DELAY);
}

void displayUnlock() {
    if (displayMutex) xSemaphoreGiveRecursive(displayMutex);
}

// ---------------------------------------------------------------------------
// Text fields: remember the last drawn text and its exact bounds so that only
// the old glyph area is cleared, and only when the text actually changed.
// ---------------------------------------------------------------------------
struct TextField {
    int16_t        cx, cy;      // cursor position
    const GFXfont *font;
    uint8_t        size;
    int16_t        bx, by;      // bounds of the last drawn text
    uint16_t       bw, bh;
    char           last[12];    // last drawn text
    bool           valid;       // false = nothing drawn yet (or screen was wiped)
};

static TextField fTargetTemp  = {102, 126, &FreeSerifBold9pt7b,   1};
static TextField fTargetHum   = {237, 126, &FreeSerifBold9pt7b,   1};
static TextField fCurrentTemp = {72,  101, &FreeSerifBold9pt7b,   2};
static TextField fCurrentHum  = {208, 100, &FreeSerifBold9pt7b,   2};
static TextField fModeName    = {232, 33,  &FreeSerifItalic12pt7b, 1};

static char    timerLast[12]  = "";
static int32_t timerLastColor = -1;
static int8_t  drawnAlarm     = -1;   // -1 = unknown, 0 = hidden, 1 = shown

static void invalidateAllFields(void) {
    TextField *fields[] = {&fTargetTemp, &fTargetHum, &fCurrentTemp, &fCurrentHum, &fModeName};
    for (TextField *f : fields) {
        f->valid   = false;
        f->last[0] = '\0';
    }
    timerLast[0]   = '\0';
    timerLastColor = -1;
    drawnAlarm     = -1;
}

// Neutral GFX state so nothing leaks into the next draw call.
static void resetTextState(void) {
    tft.setFont(nullptr);
    tft.setTextSize(1);
    tft.setTextColor(colorUI_Text);
    tft.setTextWrap(false);
}

// Returns true if the text was redrawn.
static bool drawField(TextField &f, const char *text) {
    if (f.valid && strcmp(f.last, text) == 0) return false;

    tft.setFont(f.font);
    tft.setTextSize(f.size);
    tft.setTextColor(colorUI_Text);

    if (f.valid) {
        tft.fillRect(f.bx - 1, f.by - 1, f.bw + 2, f.bh + 2, COLOR_BG);
    }

    tft.getTextBounds(text, f.cx, f.cy, &f.bx, &f.by, &f.bw, &f.bh);
    tft.setCursor(f.cx, f.cy);
    tft.print(text);

    snprintf(f.last, sizeof(f.last), "%s", text);
    f.valid = true;
    return true;
}

static void printStatic(const GFXfont *font, uint8_t size, int16_t x, int16_t y, const char *text) {
    tft.setFont(font);
    tft.setTextSize(size);
    tft.setTextColor(colorUI_Text);
    tft.setCursor(x, y);
    tft.print(text);
}

// NaN-safe, clamped integer formatting.
static void formatValue(char *buf, size_t len, float value, int minVal, int maxVal, const char *suffix) {
    if (isnan(value)) {
        snprintf(buf, len, "--%s", suffix);
        return;
    }
    int v = (int)value;
    if (v < minVal) v = minVal;
    if (v > maxVal) v = maxVal;
    snprintf(buf, len, "%d%s", v, suffix);
}

static void drawDegreeMarks(void) {
    tft.drawCircle(123, 117, 2, colorUI_Text);
    tft.drawCircle(113, 81, 3, colorUI_Text);
}

// Full static layout. Call after init and after anything wiped the screen.
void drawStaticBase(void) {
    displayLock();

    tft.fillScreen(COLOR_BG);
    tft.setTextWrap(false);

    printStatic(&FreeSerifBold9pt7b,    1, 46,  126, "Temp");
    printStatic(&FreeSerifBold9pt7b,    1, 187, 127, "Hum");
    printStatic(&FreeSerifItalic12pt7b, 1, 140, 31,  "E1");
    printStatic(&FreeSerifBold9pt7b,    1, 127, 126, "C");
    printStatic(&FreeSerifBold9pt7b,    2, 114, 102, "C");

    drawDegreeMarks();
    tft.drawBitmap(185, 75, image_weather_humidity_bits, 19, 31, colorUI_Text);
    tft.drawBitmap(49, 75, image_weather_temperature_bits, 18, 32, colorUI_Text);

    tft.drawLine(11, 46, 305, 46, colorUI_Text);
    tft.drawLine(10, 154, 304, 154, colorUI_Text);

    // Clock icon is static: drawn once here, never cleared by the timer.
    tft.drawRGBBitmap(34, 177, clock_icon, 45, 46);

    invalidateAllFields();
    resetTextState();

    displayUnlock();
}

// Safe to call often: only redraws when mode or alarm state changed.
void drawModeAndAlarm(void) {
    displayLock();

    // Mode
    int mode = (int)currentModeIndex;
    if (mode < 0 || mode > 4) mode = 0;
    fModeName.cx = (mode == 4) ? 225 : 232;
    drawField(fModeName, modeNames[mode]);

    // Alarm
    int8_t alarmOn = ActivatedAlarm ? 1 : 0;
    if (alarmOn != drawnAlarm) {
        if (alarmOn) {
            tft.drawRGBBitmap(199, 14, alarm_icon, 25, 23);
        } else {
            tft.fillRect(199, 14, 25, 23, COLOR_BG);
        }
        drawnAlarm = alarmOn;
    }

    resetTextState();
    displayUnlock();
}

// Sensors, targets and countdown timer.
void updateFastValues(void) {
    displayLock();

    char buf[12];

    // 1. Target temperature
    formatValue(buf, sizeof(buf), (float)currentTargetTemp, -9, 99, "");
    if (drawField(fTargetTemp, buf)) drawDegreeMarks();

    // 2. Target humidity
    formatValue(buf, sizeof(buf), (float)currentTargetHum, 0, 100, "%");
    drawField(fTargetHum, buf);

    // 3. Current temperature
    formatValue(buf, sizeof(buf), (float)currentTemperature, -9, 99, "");
    if (drawField(fCurrentTemp, buf)) drawDegreeMarks();

    // 4. Current humidity
    formatValue(buf, sizeof(buf), (float)currentHumidity, 0, 100, "%");
    drawField(fCurrentHum, buf);

    // 5. Countdown timer (default font is fixed width, so opaque text needs no clearing)
    long total = (long)remainingSeconds;
    if (total < 0) total = 0;
    if (total > 359999) total = 359999;   // 99:59:59 max

    char timeStr[12];
    snprintf(timeStr, sizeof(timeStr), "%02ld:%02ld:%02ld",
             total / 3600, (total % 3600) / 60, total % 60);

    uint16_t timerColor = systemRunning ? colorUI_Active : colorUI_Inactive;

    if (strcmp(timeStr, timerLast) != 0 || (int32_t)timerColor != timerLastColor) {
        tft.setFont(nullptr);
        tft.setTextSize(4);
        tft.setTextColor(timerColor, COLOR_BG);
        tft.setCursor(85, 190);
        tft.print(timeStr);
        snprintf(timerLast, sizeof(timerLast), "%s", timeStr);
        timerLastColor = timerColor;
    }

    resetTextState();
    displayUnlock();
}

static void redrawAll(void) {
    displayLock();
    drawStaticBase();
    drawModeAndAlarm();
    updateFastValues();
    displayUnlock();
}

void displayTask(void *pvParameters) {
    displayLock();
    tft.init(240, 320);
    tft.setRotation(1);
    displayUnlock();

    redrawAll();

    unsigned long lastValuesUpdate = 0;
    unsigned long lastIconUpdate   = 0;
    bool wasFrozen = false;

    for (;;) {
        unsigned long now = millis();

        // Freeze the UI while the settings menu or the warning is on screen
        // (actuators and timer keep running in the background).
        bool frozen = inMenu || modeWarningActive;

        if (frozen) {
            wasFrozen = true;
        } else {
            // The menu draws over the screen: rebuild everything when it closes.
            if (wasFrozen) {
                redrawAll();
                lastValuesUpdate = now;
                wasFrozen = false;
            }

            if (now - lastValuesUpdate >= 200) {
                drawModeAndAlarm();
                updateFastValues();
                lastValuesUpdate = now;
            }

            if (now - lastIconUpdate >= 125) {
                displayLock();
                runIcon(flameIcon, tft);
                runIcon(fanIcon, tft);
                runIcon(humidifierIcon, tft);
                displayUnlock();
                lastIconUpdate = now;
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void initDisplayTask() {
    displayMutex = xSemaphoreCreateRecursiveMutex();
    xTaskCreate(displayTask, "DisplayTask", 8192, NULL, 1, NULL);
}