#include "Configuracoes.h" // 1º Pinos
#include "Variaveis.h"     // 2º Memória
#include "Funcoes.h"       // 3º Ações

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SSR, OUTPUT);
  digitalWrite(PIN_SSR, LOW);
  pinMode(PIN_BYPASS, OUTPUT);
  digitalWrite(PIN_BYPASS, LOW);
  pinMode(pinoSair, OUTPUT);
  digitalWrite(pinoSair, LOW);
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  somSerial.begin(9600, SERIAL_8N1, 16, 17);
  if (myDFPlayer.begin(somSerial)) { myDFPlayer.volume(volumeAtual); }

  SPI.begin(18, 19, 23);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(0x0000);

  pinMode(PIN_BUZZER, OUTPUT);
  ledcAttachChannel(PIN_BUZZER, 2000, 10, 1);
  ledcAttach(PIN_FAN, FREQ_FAN, RESOLUCAO_FAN);
  ledcWrite(PIN_FAN, map(10, 0, 100, 0, 255));

  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(PIN_PEDAL, INPUT_PULLUP);
  pinMode(PIN_CANETA, INPUT);
  analogReadResolution(12);

  prefs.begin("soldas", false);
  soldasTotais = prefs.getUInt("count", 0);
  minutosTotais = prefs.getUInt("uptime", 0);
  volumeSistema = prefs.getInt("volume", 70);
  prefs.end();

  WiFi.mode(WIFI_STA);

  if (SD.begin(SD_CS)) {
    estado = SPLASH1;
    if (SD.exists("/imagem1.bmp")) reader.drawBMP("/imagem1.bmp", tft, 0, 0);
    somGemini();
    tSplash = millis();
    carregarWiFiDoSD();
  } else {
    estado = PRONTO;
    desenhaPainelBase();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  configTime(-10800, 0, "pool.ntp.org", "time.google.com");

  esp_task_wdt_config_t twdt = { .timeout_ms = 30000, .idle_core_mask = 3, .trigger_panic = false };
  esp_task_wdt_init(&twdt);
  esp_task_wdt_add(NULL);
}

void loop() {
  esp_task_wdt_reset();

  static unsigned long tRede = 0;
  if (estado != SOLDANDO && (millis() - tRede > 10000)) {
    tRede = millis();
    if (WiFi.status() == WL_CONNECTED) {
      ArduinoOTA.handle();
    } else {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }

  if (estado == SPLASH1 || estado == SPLASH2 || estado == SPLASH3) {
    if (millis() - tSplash > 3000 || (lerBotao() && millis() > 1000)) {
      tSplash = millis();
      if (estado == SPLASH1) {
        estado = SPLASH2;
        tft.fillScreen(0x0000);
        if (SD.exists("/imagem2.bmp")) reader.drawBMP("/imagem2.bmp", tft, 0, 0);
        somHarmonicoA();
      } else if (estado == SPLASH2) {
        estado = SPLASH3;
        tft.fillScreen(0x0000);
        if (SD.exists("/imagem3.bmp")) reader.drawBMP("/imagem3.bmp", tft, 0, 0);
        somHarmonicoB();
      } else {
        estado = PRONTO;
        somGemini();
        desenhaPainelBase();
      }
    }
    return;
  }

  if (estado == SOLDANDO) {
    unsigned long decorrido = millis() - tSoldaInicio;
    int p1 = materiais[materialIndex].p1;
    int p2 = (materialIndex == 6) ? customPercent * 5 : materiais[materialIndex].p2;
    int intervalo = materiais[materialIndex].intervalo;

    if (faseSolda == 1 && decorrido >= p1) {
      digitalWrite(PIN_SSR, LOW);
      faseSolda = 2;
    } else if (faseSolda == 2 && decorrido >= (p1 + intervalo)) {
      digitalWrite(PIN_SSR, HIGH);
      faseSolda = 3;
    } else if (faseSolda == 3 && decorrido >= (p1 + intervalo + p2)) {
      digitalWrite(PIN_SSR, LOW);
      faseSolda = 0;
      soldasTotais++;
      estado = PRONTO;
      desenhaPainelBase();
    }
    return;
  }

  if (estado == PRONTO) {
    atualizarRelogioESinal();

    if (millis() - tDados > INTERVALO_ATUALIZACAO_PAINEL) {
      tDados = millis();
      renderizarSensor(0, NTC_CABO);
      renderizarSensor(1, NTC_TRAFO);
      renderizarSensor(2, NTC_SSR);
      atualizarLeituraPAC();
      logicaAnimacaoCooler();

      tft.fillRect(10, 198, 140, 25, 0x0000);
      tft.setCursor(10, 198);
      tft.setTextSize(2);
      uint16_t corMenu = ajustandoCustom ? 0xFFE0 : 0x07FF;
      tft.setTextColor(corMenu);

      if (materialIndex == 6) tft.printf("> Cust:%d%%", customPercent);
      else if (materialIndex == 10) tft.printf("> VOL:%d", volumeAtual);
      else tft.printf("> %s", materiais[materialIndex].nome);
    }

    int dir = 0;
    if (lerEncoder(&dir)) {
      if (ajustandoCustom) {
        if (materialIndex == 6) customPercent = constrain(customPercent + dir, 5, 100);
        if (materialIndex == 10) {
          volumeAtual = constrain(volumeAtual + dir, 0, 30);
          myDFPlayer.volume(volumeAtual);
        }
      } else {
        materialIndex += dir;
        if (materialIndex < 0) materialIndex = 12;
        if (materialIndex > 12) materialIndex = 0;
      }
      bipEncoder(dir);
      tDados = 0;
    }

    if (lerBotao()) {
      if (materialIndex == 6 || materialIndex == 10) {
        ajustandoCustom = !ajustandoCustom;
        bipSuave();
      } else if (materialIndex == 7) {
        unsigned long tInicioAperto = millis();
        bool longo = false;
        while (digitalRead(ENC_SW) == LOW) {
          if (millis() - tInicioAperto > 2000) { longo = true; break; }
          delay(10);
        }
        if (longo) iniciarWPS();
        else abrirTelaWiFi();
      } else if (materialIndex == 8) abrirTelaZerarProducao();
      else if (materialIndex == 9) abrirTelaAtuliz();
      else if (materialIndex == 11) abrirTelaBypass();
      else if (materialIndex == 12) executarSaidaSistema();
      tDados = 0;
    }

    if (digitalRead(PIN_CANETA) == LOW && materialIndex < 7) {
      if (digitalRead(PIN_PEDAL) == LOW && pedalLiberado) {
        pedalLiberado = false;
        estado = SOLDANDO;
        faseSolda = 1;
        tSoldaInicio = millis();
        digitalWrite(PIN_SSR, HIGH); 
        if (SD.exists("/soldando.bmp")) reader.drawBMP("/soldando.bmp", tft, 0, 0);
      }
    }
  }

  if (digitalRead(PIN_PEDAL) == HIGH) { pedalLiberado = true; }

  static unsigned long tSave = 0;
  if (millis() - tSave > 60000) {
    tSave = millis();
    prefs.begin("soldas", false);
    prefs.putUInt("count", soldasTotais);
    prefs.putUInt("uptime", minutosTotais);
    prefs.putInt("volume", volumeAtual);
    prefs.end();
  }
}