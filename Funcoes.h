// ============================================================================
// GAVETA DE FERRAMENTAS - TODA A LÓGICA DO SP-01 GOLD
// ============================================================================

void tocarSom(int frequencia, int duracao) {
  if (volumeSistema <= 0) { ledcWrite(PIN_BUZZER, 0); return; }
  ledcWriteTone(PIN_BUZZER, frequencia);
  ledcWrite(PIN_BUZZER, map(volumeSistema, 0, 100, 0, 512));
  delay(duracao);
  ledcWrite(PIN_BUZZER, 0); ledcWriteTone(PIN_BUZZER, 0);
}

void somGemini() { int notas[] = {698, 880, 1046, 1318, 1568}; for(int i=0; i<5; i++){ tocarSom(notas[i], 120); delay(20); } }
void somHarmonicoA() { tocarSom(1760, 80); delay(20); tocarSom(2093, 80); }
void somHarmonicoB() { tocarSom(1318, 80); delay(20); tocarSom(1046, 80); delay(20); tocarSom(1318, 150); }
void bipEncoder(int dir) { if (volumeSistema > 0) tocarSom(dir > 0 ? 4200 : 3800, 8); }
void bipSuave() { if (bipHabilitado) tocarSom(2200, 30); }

uint16_t obterCorDegrade(int i, int total) {
  float percent = (float)i / total;
  uint8_t r, g;
  if (percent < 0.5) { r = map(percent*100, 0, 50, 0, 31); g = 63; }
  else { r = 31; g = map(percent*100, 50, 100, 63, 0); }
  return (r << 11) | (g << 5);
}

bool lerBotao() {
  if (digitalRead(ENC_SW) == LOW) {
    unsigned long t = millis();
    while (digitalRead(ENC_SW) == LOW) { if (millis()-t > 2000) return true; delay(5); }
    if (millis()-t > 50) return true;
  }
  return false;
}

bool lerEncoder(int *delta) {
  static int lastCLK = HIGH;
  int clk = digitalRead(ENC_CLK);
  if (clk != lastCLK && clk == LOW) {
    *delta = (digitalRead(ENC_DT) != clk) ? 1 : -1;
    lastCLK = clk; return true;
  }
  lastCLK = clk; return false;
}

float lerTemperatura(int pino) {
  float reading = analogRead(pino);
  if (reading < 1) return -999;
  float resistance = 10000.0 / (4095.0 / reading - 1);
  float steinhart = log(resistance / 10000.0) / 3950.0 + 1.0 / (25.0 + 273.15);
  return (1.0 / steinhart) - 273.15;
}

void renderizarSensor(int pos, int pino) {
  float t = lerTemperatura(pino);
  int y = 62 + (pos * 44);
  tft.setTextColor(0xFFFF, 0x0000); tft.setTextSize(3); tft.setCursor(105, y+5);
  if (t < -900) tft.print("OFF "); else tft.printf("%2.0f", t);
}

void logicaAnimacaoCooler() {
  tempCaboGlobal = lerTemperatura(NTC_CABO);
  tempTrafGlobal = lerTemperatura(NTC_TRAFO);
  tempSSRGlobal = lerTemperatura(NTC_SSR);
  fanState = (tempSSRGlobal > 85 || tempTrafGlobal > 90) ? 100 : 20;
  ledcWrite(PIN_FAN, map(fanState, 0, 100, 0, 255));
}

void desenhaPainelBase() {
  tft.fillScreen(0x0000);
  tft.drawFastVLine(180, 55, 140, 0xFD20);
  if (SD.exists("/imagem4.bmp")) reader.drawBMP("/imagem4.bmp", tft, 195, 56);
}

void atualizarRelogioESinal() {
  tft.setTextSize(2); tft.setTextColor(0x07E0, 0x0000);
  tft.setCursor(220, 10); tft.print("SP-01");
}

void carregarWiFiDoSD() { WiFi.begin(WIFI_SSID, WIFI_PASS); }

// --- COLOQUE AQUI AS FUNÇÕES DE TELA QUE JÁ TINHA ---
void abrirTelaVolume() { /* código que já enviou */ }
void abrirTelaBypass() { /* código que já enviou */ }
void atualizarLeituraPAC() { /* código que já enviou */ }
void abrirTelaWiFi() { tft.fillScreen(0x0000); tft.print("WiFi Info"); delay(2000); }
void abrirTelaAtuliz() { tft.fillScreen(0x0000); tft.print("Update Menu"); delay(2000); }
void abrirTelaZerarProducao() { soldasTotais = 0; }
void executarSaidaSistema() { ESP.restart(); }
void iniciarWPS() { somHarmonicoA(); }