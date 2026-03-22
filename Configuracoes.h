#ifndef CONFIG_H
#define CONFIG_H

#include <SdFat.h>
#include <Adafruit_ImageReader.h>
#include <WiFi.h>
#include <time.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "DFRobotDFPlayerMini.h"
#include "esp_wps.h" 

// --- DEFINIÇÃO DE PINOS ---
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4
#define SD_CS 5
#define PIN_SSR 22
#define PIN_BUZZER 21
#define PIN_PEDAL 26
#define ENC_CLK 32
#define ENC_DT  33
#define ENC_SW  25
#define NTC_CABO 39
#define NTC_TRAFO 35
#define NTC_SSR 36
#define PIN_FAN 27
#define PIN_CANETA 34
#define PIN_BYPASS 13
#define PIN_PAC_READ 14
#define PIN_SHUTDOWN 12

#define FREQ_FAN 25000
#define RESOLUCAO_FAN 8
#define INTERVALO_ATUALIZACAO_PAINEL 400

#endif