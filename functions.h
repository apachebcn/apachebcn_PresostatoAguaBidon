#pragma once

#include <Arduino.h>

// Variables globales compartidas con el resto del sketch.
// Las DEFINICIONES están en PresostatoAguaBidon.ino; aquí solo se declaran como
// extern para que este módulo (functions.cpp) pueda usarlas.
extern bool valueSwitchLedsHigh;
extern bool valueSwitchForceRelayOn;

extern bool intermitentEnable;
extern bool intermitentStepOn;

extern bool leakAlarm;
extern bool alarm;
extern bool relay_on;
extern unsigned long relayOnSince;

extern bool levelValue;
extern bool levelRawValue;
extern unsigned long levelSamplesOn;
extern unsigned long levelSamplesOff;

extern bool electrode1Value;
extern bool electrode2Value;

// Prototipos de las funciones del módulo (implementadas en functions.cpp)
void serial_println();
void serial_println(String message);
void updateBrightLeds();
void updateLeds();
void ledOn(unsigned int pin_led);
void ledOff(unsigned int pin_led);
void ledsAllOn();
void ledsAllOff();
void ledsTest();
bool getRelayActionOn(bool switchForceRelayOn, bool switchForceRelayOff, bool levelValue, bool electrode1Value, bool electrode2Value);
bool relayRulesApply();
bool alarmRulesApply();
void refreshRelay(bool relay_on);
void applyLeakAlarm();
void checkLeakAlarm();
bool isSwitchChanged(bool valueSwitch, bool lastState);
void levelRead();
bool levelReadApply();
unsigned int analogReadShortPulse(unsigned int pinPulse, unsigned int pinAnalog, unsigned int pulseTimeUs);
void electrode1Read();
bool electrode1ReadApply();
void electrode2Read();
bool electrode2ReadApply();
void beep(int pin, int frequency, int duration_ms);
void leak_alarm_sound();
void alarm_sound();
