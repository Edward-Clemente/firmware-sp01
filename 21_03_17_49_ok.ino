
// ============================================================================
// CONFIGURAÇÕES DE HARDWARE E PINAGEM - SP-01 GOLD
// ============================================================================
// Aqui definimos para onde cada fio vai no ESP32.
// SSR (22): Onde disparamos a solda.
// PEDAL (26): Entrada do acionamento físico.
// ENCODER (32, 33, 25): Navegação e cliques do menu.
// NTCs (39, 35, 36): Monitoramento térmico de segurança.
// FAN (27): Controle de resfriamento via PWM.
// PAC (14): Leitura da voltagem das baterias 2S.

// --- REMOVIDO WiFiClientSecure.h para aliviar a Flash ---
// O OTA e Update Local continuam funcionando via WiFi.h e HTTPUpdate.h
/*
#include <SdFat.h>
#include <Adafruit_ImageReader.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include "DFRobotDFPlayerMini.h"
#include "esp_wps.h"  // Biblioteca oficial para o botão do roteador*/

// ============================================================================
// CONFIGURAÇÕES DE HARDWARE E PINAGEM - SP-01 GOLD
// ============================================================================
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

// --- PROTÓTIPOS DE FUNÇÕES (AVISO PRÉVIO AO COMPILADOR) ---
bool lerEncoder(int *delta);
void bipEncoder();
void salvarConfiguracoes();
void carregarConfiguracoes();
float lerTemperatura(int pino);
void abrirTelaAtuliz();
void abrirTelaZerarProducao();
void abrirTelaVolume();
void abrirTelaBypass();
void executarSaidaSistema();

// Protótipos de funções (Avisa ao ESP32 que essas funções existem lá embaixo)
bool lerEncoder(int* delta);
void bipEncoder();
void abrirTelaAtuliz();

// Configurações do WPS
static esp_wps_config_t configWps;
bool wpsEmAndamento = false;

HardwareSerial somSerial(2);  // Usa a Serial 2 do ESP32
DFRobotDFPlayerMini myDFPlayer;

int volumeAtual = 25;  // Volume inicial alto para os 20 ohms

// --- PROTÓTIPOS DE FUNÇÕES (Para evitar erros de declaração) ---

bool lerBotao();
void desenhaPainelBase();
float lerTemperatura(int pino);

// --- CONFIGURAÇÕES DE HARDWARE E PINAGEM - SP-01 GOLD ---
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



// --- CONSTANTES DO SISTEMA ---
#define FREQ_FAN 25000
#define RESOLUCAO_FAN 8

// --- DEFINIÇÕES DE HARDWARE E SAÍDAS ---
const int pinoSair = 0;  // ÚNICA DEFINIÇÃO: pino 0 liberando o 16 para o Som

bool bypassAtivo = false;
// --- VERSÃO E REPOSITÓRIO ---
const char* VERSAO_ATUAL = "1.1.4";
const String updateUrl = "https://raw.githubusercontent.com/Edward-Clemente/firmware-sp01/main/up.bin";
//const String IP_LOCAL_PC = "http://PC-EDWARD.local/atualizar/up.bin";
const String IP_LOCAL_PC = "http://192.168.137.1/atualizar/up.bin";

const float FATOR_CALIBRACAO = 2.51;  // Este número calibra o Volts do display com o seu Multímetro
float tensaoPAC = 0.0;
int porcentagemPAC = 0;

// --- CONFIGURAÇÕES ---
const float RESISTOR_SERIE = 10000.0, NTC_NOMINAL = 10000.0, TEMP_NOMINAL = 25.0, BETA_COEF = 3435.0, ADC_MAX = 4095.0, OFFSET_GERAL = -6.0;
#define INTERVALO_ATUALIZACAO_PAINEL 400
const char* WIFI_SSID = "Monte Alto_EXT";
const char* WIFI_PASS = "21216666Aa";

// --- VARIÁVEIS GLOBAIS ---
SdFat SD;
Adafruit_ImageReader reader(SD);
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;

int pwmAtual = 10;
int pwmAlvo = 10;
unsigned long tempoRampa = 0;

unsigned long tPassoCooler = 0;
int modoCiclo = 0;

int volumeSistema = 70;  // 0 a 100
bool telaVolumeAtiva = false;

// ===== CONTROLE CICLICO INDUSTRIAL DO FAN =====
int etapaFan = 10;
bool subindoFan = true;
unsigned long tempoUltimoPassoFan = 0;
unsigned long tempoMaximoFan = 0;
bool segurandoNoMaximo = false;

float tempSSRGlobal = 0;
float tempTrafGlobal = 0;
float tempCaboGlobal = 0;

int fanState = 10;  // guarda estado atual real do fan

struct Material {
  const char* nome;
  uint16_t p1, p2, intervalo;
};

Material materiais[] = {
  { "Ni 0.10", 10, 40, 100 }, { "Ni 0.12", 15, 60, 150 }, { "Ni 0.15", 20, 90, 200 }, { "Ni 0.18", 25, 130, 250 }, { "ACO NIQ", 30, 180, 300 }, { "COBRE", 50, 400, 400 }, { "CUST", 15, 60, 200 }, { "WIFI", 0, 0, 0 }, { "PRODUCAO", 0, 0, 0 }, { "ATUALIZ.", 0, 0, 0 }, { "SOM: ON", 0, 0, 0 }, { "BYPASS", 0, 0, 0 },  // <--- NOVA OPÇÃO (Índice 11)
  { "SAIR", 0, 0, 0 }                                                                                                                                                                                                                                                                                                      // <--- AGORA É ÍNDICE 12
};

enum Estado { SPLASH1,
              SPLASH2,
              SPLASH3,
              PRONTO,
              SOLDANDO,
              TELA_WIFI,
              TELA_PROD,
              TELA_CONF,
              TELA_UPDATING };
Estado estado = SPLASH1;

int materialIndex = 0, customPercent = 15, velocidadeFanAtual = 10, corIndice = 0;
int ultimaVelocidadeFan = -1;
bool ajustandoCustom = false, pedalLiberado = true, bipHabilitado = true;
uint32_t soldasTotais = 0, minutosTotais = 0;
unsigned long tSplash = 0, tDados = 0, tUltimoSalvo = 0, ultimoDebounce = 0, ultimoDebounceSW = 0;

int lastCLK = HIGH;
uint16_t coresTitulo[] = { 0xFFFF, 0x07FF, 0x07E0, 0xF81F, 0xFD20, 0x07E0 };
unsigned long tSoldaInicio = 0;
int faseSolda = 0;

// --- FUNÇÃO: SALVAR REDE NO SD ---
void salvarWiFiNoSD(String ssid, String pass) {
  File arquivo = SD.open("/wifi_cfg.txt", FILE_WRITE);
  if (arquivo) {
    arquivo.println(ssid);
    arquivo.println(pass);
    arquivo.close();
    Serial.println("Rede salva no SD!");
  }
}

// --- FUNÇÃO: CARREGAR REDE DO SD ---
void carregarWiFiDoSD() {
  if (SD.exists("/wifi_cfg.txt")) {
    File arquivo = SD.open("/wifi_cfg.txt", FILE_READ);
    if (arquivo) {
      String s = arquivo.readStringUntil('\n');
      String p = arquivo.readStringUntil('\n');
      s.trim();
      p.trim();
      if (s.length() > 0) {
        WiFi.begin(s.c_str(), p.c_str());
        arquivo.close();
        return;
      }
      arquivo.close();
    }
  }
  WiFi.begin("Monte Alto_EXT", "21216666Aa");  // Backup padrão
}

// --- FUNÇÃO: MODO PAREAMENTO WPS ---
void iniciarWPS() {
  tft.fillScreen(0x0000);
  for (int i = 0; i < 6; i++) tft.drawRoundRect(15 + i, 15 + i, 290 - (2 * i), 210 - (2 * i), 25, 0xFD20);

  tft.setTextSize(2);
  tft.setTextColor(0xFFFF);
  tft.setCursor(40, 70);
  tft.println("MODO PAREAMENTO WPS");
  tft.setTextColor(0x07FF);
  tft.setCursor(40, 110);
  tft.println("APERTE O BOTAO 'WPS'");
  tft.setCursor(40, 135);
  tft.println("NO SEU ROTEADOR AGORA");

  // Configura o WPS
  configWps.wps_type = WPS_TYPE_PBC;
  esp_wifi_wps_enable(&configWps);
  esp_wifi_wps_start(0);

  unsigned long startWPS = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startWPS < 60000) {
    esp_task_wdt_reset();
    tft.fillRect(100, 170, 120, 10, 0x0000);
    tft.setCursor(120, 170);
    tft.print("AGUARDANDO...");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    salvarWiFiNoSD(WiFi.SSID(), WiFi.psk());
    tft.fillScreen(0x07E0);
    tft.setTextColor(0x0000);
    tft.setCursor(60, 110);
    tft.print("CONECTADO E SALVO!");
    somHarmonicoA();
    delay(3000);
  } else {
    tft.fillScreen(0xF800);
    tft.setCursor(60, 110);
    tft.print("FALHA NO WPS");
    delay(3000);
  }
}

// ============================================================================
// SISTEMA DE ÁUDIO COM CONTROLE DE AMPLITUDE (PWM)
// ============================================================================

void tocarSom(int frequencia, int duracao) {
  if (volumeSistema <= 0) {
    ledcWrite(PIN_BUZZER, 0);
    return;
  }

  // O segredo para o som não "rasgar":
  ledcWriteTone(PIN_BUZZER, frequencia);
  int dutyReal = map(volumeSistema, 0, 100, 0, 512);  // Volume real
  ledcWrite(PIN_BUZZER, dutyReal);

  delay(duracao);

  ledcWrite(PIN_BUZZER, 0);      // Desliga o volume
  ledcWriteTone(PIN_BUZZER, 0);  // Desliga a nota
}

void somGemini() {
  if (volumeSistema == 0) return;
  int notas[] = { 698, 880, 1046, 1318, 1568 };
  for (int i = 0; i < 5; i++) {
    tocarSom(notas[i], 120);
    delay(20);
  }
}

void somHarmonicoA() {
  tocarSom(1760, 80);
  delay(20);
  tocarSom(2093, 80);
}

void somHarmonicoB() {
  tocarSom(1318, 80);
  delay(20);
  tocarSom(1046, 80);
  delay(20);
  tocarSom(1318, 150);
}

void bipEncoder(int dir) {
  if (volumeSistema == 0) return;
  tocarSom(dir > 0 ? 4200 : 3800, 8);
}

void bipSuave() {
  if (bipHabilitado) { tocarSom(2200, 30); }
}

void gerenciarBipContato() {
  if (volumeSistema == 0 || estado == SOLDANDO) return;
  static unsigned long tUltimoBip = 0;
  if (millis() - tUltimoBip > 350) {
    tocarSom(2800, 15);
    tUltimoBip = millis();
  }
}

uint16_t obterCorDegrade(int i, int total) {
  float percent = (float)i / total;
  uint8_t r, g, b;
  if (percent < 0.5) {
    r = (uint8_t)(map(percent * 100, 0, 50, 0, 31));
    g = 63;
    b = 0;
  } else {
    r = 31;
    g = (uint8_t)(map(percent * 100, 50, 100, 63, 0));
    b = 0;
  }
  return (r << 11) | (g << 5) | b;
}

void abrirTelaInfoSistema() {
  estado = TELA_CONF;
  tft.fillScreen(0x0000);
  // Moldura Laranja Premium
  for (int i = 0; i < 6; i++) tft.drawRoundRect(15 + i, 15 + i, 290 - (2 * i), 210 - (2 * i), 25, 0xFD20);

  tft.setTextSize(2);
  tft.setTextColor(0x07FF);
  tft.setCursor(65, 45);
  tft.print("INFO DO SISTEMA");
  tft.drawFastHLine(50, 65, 220, 0x3186);

  // Aqui o Horímetro aparece com destaque!
  tft.setTextSize(3);
  tft.setTextColor(0xFFFF);
  tft.setCursor(60, 95);
  tft.print("USO:");
  tft.setTextColor(0xFD20);
  tft.printf(" %dh %02dm", minutosTotais / 60, minutosTotais % 60);

  tft.setTextSize(2);
  tft.setTextColor(0xFFFF);
  tft.setCursor(60, 145);
  tft.printf("TOTAL SOLDAS: %lu", soldasTotais);
  tft.setCursor(60, 175);
  tft.printf("PAC ATUAL: %2.2fV", tensaoPAC);

  while (!lerBotao()) {
    esp_task_wdt_reset();
    delay(10);
  }
  estado = PRONTO;
  desenhaPainelBase();
}

// Esta função encapsula toda a lógica visual que você estava tentando inserir
void atualizarDinamicaCooler() {
  if (estado != PRONTO) return;

  // --- 1. MOLDURA DO FAN ---
  int xBox = 190;
  int yBox = 144;
  int wBox = 120;
  int hBox = 28;

  if (velocidadeFanAtual != ultimaVelocidadeFan) {
    tft.fillRect(xBox + 2, yBox + 2, wBox - 4, hBox - 4, 0x0000);
    tft.drawRoundRect(xBox, yBox, wBox, hBox, 6, 0xFFE0);
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setCursor(xBox + 18, yBox + 7);
    tft.printf("FAN %d%%", velocidadeFanAtual);

    // --- 2. BARRA VERTICAL LATERAL (COCKPIT) ---
    int xB = 285, yB = 58, hB = 70, wB = 18;
    tft.drawRect(xB, yB, wB, hB, 0xFFFF);
    int altP = map(velocidadeFanAtual, 0, 100, 0, hB - 2);

    tft.fillRect(xB + 1, yB + 1, wB - 2, hB - 2, 0x0000);

    for (int i = 0; i < altP; i++) {
      int yL = yB + hB - 2 - i;
      float p = (float)i / (hB - 2);
      uint16_t cor;
      if (p < 0.5) {
        uint8_t g = 63;
        uint8_t r = map(p * 100, 0, 50, 0, 31);
        cor = (r << 11) | (g << 5) | 0;
      } else {
        uint8_t r = 31;
        uint8_t g = map(p * 100, 50, 100, 63, 0);
        cor = (r << 11) | (g << 5) | 0;
      }
      tft.drawFastHLine(xB + 1, yL, wB - 2, cor);
    }
    ultimaVelocidadeFan = velocidadeFanAtual;
  }
}

// ============================================================================
// GESTÃO TÉRMICA E CICLO DE EXPURGO (FAN)
// ============================================================================

void logicaAnimacaoCooler() {
  // 1. PRIORIDADE TOTAL À SOLDA: Se estiver soldando, sai imediatamente.
  if (estado == SOLDANDO) return;

  static unsigned long tUltimaLeitura = 0;
  static int sensorDaVez = 0;  // 0=Cabo, 1=Trafo, 2=SSR
  unsigned long agora = millis();

  // 2. LEITURA ESCALONADA: Lê um sensor a cada 150ms para não sobrecarregar o loop
  if (agora - tUltimaLeitura > 150) {
    tUltimaLeitura = agora;

    switch (sensorDaVez) {
      case 0: tempCaboGlobal = lerTemperatura(NTC_CABO); break;
      case 1: tempTrafGlobal = lerTemperatura(NTC_TRAFO); break;
      case 2: tempSSRGlobal = lerTemperatura(NTC_SSR); break;
    }

    sensorDaVez++;
    if (sensorDaVez > 2) sensorDaVez = 0;
  }

  static int ultimoFan = -1;

  // 3. PROTEÇÃO TÉRMICA PRIORITÁRIA
  if (tempSSRGlobal > 85 || tempTrafGlobal > 90 || tempCaboGlobal > 70) {
    fanState = 100;
  }
  // Se algum sensor estiver desconectado (OFF), mantém rotação de segurança
  else if (tempSSRGlobal < -900 || tempTrafGlobal < -900 || tempCaboGlobal < -900) {
    fanState = 40;
  } else {
    // 4. LÓGICA AIR FLUSH (NÃO-BLOQUEANTE)
    if (segurandoNoMaximo) {
      if (agora - tempoMaximoFan >= 15000) {
        segurandoNoMaximo = false;
        subindoFan = false;
        tempoUltimoPassoFan = agora;
      }
    } else if (agora - tempoUltimoPassoFan >= 2000) {
      tempoUltimoPassoFan = agora;
      if (subindoFan) {
        etapaFan += 10;
        if (etapaFan >= 100) {
          etapaFan = 100;
          segurandoNoMaximo = true;
          tempoMaximoFan = agora;
        }
      } else {
        etapaFan -= 10;
        if (etapaFan <= 10) {
          etapaFan = 10;
          subindoFan = true;
        }
      }
    }
    fanState = etapaFan;
  }

  // 5. ATUALIZAÇÃO INTELIGENTE: Só mexe no PWM e Display se houver mudança real
  velocidadeFanAtual = fanState;
  if (velocidadeFanAtual != ultimoFan) {
    pwmAlvo = velocidadeFanAtual;
    // Converte 0-100% para 0-255 do PWM
    ledcWrite(PIN_FAN, map(pwmAlvo, 0, 100, 0, 255));
    atualizarDinamicaCooler();
    ultimoFan = velocidadeFanAtual;
  }
}

// ============================================================================
// GESTÃO DE ATUALIZAÇÕES REMOTAS (OTA & HTTP UPDATE)
// ============================================================================

void executarUpdate(String url) {
  estado = TELA_UPDATING;
  digitalWrite(PIN_SSR, LOW);  // Segurança total
  tft.fillScreen(0x0000);

  // Moldura premium fixa
  tft.drawRoundRect(18, 98, 284, 44, 8, 0xFFFF);
  tft.drawRoundRect(19, 99, 282, 42, 7, 0x3186);

  tft.setCursor(55, 70);
  tft.setTextSize(2);
  tft.setTextColor(0x07FF);
  tft.print("BAIXANDO UPDATE...");

  static int ultimaLarguraFisica = 0;
  ultimaLarguraFisica = 0; 

  httpUpdate.onProgress([](int cur, int total) {
    int larguraTotal = 276;
    int xBarra = 22;
    int yBarra = 102;
    int alturaBarra = 36;

    int progressoAtualFisico = map(cur, 0, total, 0, larguraTotal);
    int progPercent = (cur * 100) / total;

    static int ultimaLarguraDesenhadada = -1;
    if (progressoAtualFisico > ultimaLarguraDesenhadada) {
      for (int i = (ultimaLarguraDesenhadada < 0 ? 0 : ultimaLarguraDesenhadada); i < progressoAtualFisico; i++) {
        float p = (float)i / larguraTotal;
        uint16_t cor;
        if (p < 0.33) cor = tft.color565(map(p * 100, 0, 33, 0, 255), 255, 0);
        else if (p < 0.66) cor = tft.color565(255, map(p * 100, 33, 66, 255, 165), 0);
        else cor = tft.color565(255, map(p * 100, 66, 100, 165, 0), 0);

        tft.drawFastVLine(xBarra + i, yBarra, alturaBarra, cor);
      }
      ultimaLarguraDesenhadada = progressoAtualFisico;

      static int lastP = -1;
      if (progPercent != lastP) {
        tft.fillRect(130, 150, 80, 25, 0x0000);
        tft.setTextSize(2);
        tft.setTextColor(0xFFFF);
        tft.setCursor(135, 155);
        tft.printf("%d%%", progPercent);
        lastP = progPercent;
      }
    }
    esp_task_wdt_reset();
  });

  httpUpdate.rebootOnUpdate(true);
  
  // CORREÇÃO AQUI: t_httpUpdate_return com o cliente WiFi padrão
  WiFiClient client;
  t_httpUpdate_return ret = httpUpdate.update(client, url);

  if (ret == HTTP_UPDATE_FAILED || ret == HTTP_UPDATE_NO_UPDATES) {
    tft.fillScreen(0x0000);
    tft.setTextColor(0xF800);
    tft.setTextSize(2);
    tft.setCursor(20, 80);
    tft.print("ERRO NA GRAVACAO");
    tft.setCursor(20, 120);
    tft.setTextSize(1);
    tft.printf("Causa: %s (Cod:%d)", httpUpdate.getLastErrorString().c_str(), httpUpdate.getLastError());
    delay(5000);
    estado = PRONTO;
    desenhaPainelBase();
  }
}

// ============================================================================
// TELA DE SELEÇÃO DE ORIGEM DA ATUALIZAÇÃO
// ============================================================================
void abrirTelaAtuliz() {
  estado = TELA_CONF;
  int sel = 0;
  bool sair = false;

  tft.fillScreen(0x0000);
  for (int i = 0; i < 6; i++) tft.drawRoundRect(15 + i, 15 + i, 290 - (2 * i), 210 - (2 * i), 25, 0xF81F);
  tft.drawRoundRect(90, 32, 140, 32, 8, 0x07FF);
  tft.setTextSize(2);
  tft.setTextColor(0xFFFF);
  tft.setCursor(105, 40);
  tft.print("V:");
  tft.print(VERSAO_ATUAL);
  tft.drawFastHLine(50, 80, 220, 0x3186);

  auto atualizaTexto = [&]() {
    tft.setTextSize(3);
    tft.setCursor(60, 105);
    tft.setTextColor(sel == 0 ? 0xFFFF : 0x4444, 0x0000);
    tft.print(sel == 0 ? "> GITHUB" : "  GITHUB");

    tft.setCursor(60, 145);
    tft.setTextColor(sel == 1 ? 0xFFFF : 0x4444, 0x0000);
    tft.print(sel == 1 ? "> LOCAL " : "  LOCAL ");

    tft.setCursor(60, 190);
    tft.setTextColor(sel == 2 ? 0xFFE0 : 0x4444, 0x0000);
    tft.print(sel == 2 ? "> SAIR  " : "  SAIR  ");
  };

  atualizaTexto();

  while (!sair) {
    esp_task_wdt_reset();
    int d = 0;
    if (lerEncoder(&d)) {
      sel += d;
      if (sel < 0) sel = 2;
      if (sel > 2) sel = 0;
      bipEncoder(d);
      atualizaTexto();
    }

    if (lerBotao()) {
      if (sel == 0) { executarUpdate(updateUrl); sair = true; }
      else if (sel == 1) { executarUpdate(IP_LOCAL_PC); sair = true; }
      else if (sel == 2) { sair = true; }
    }
    delay(5);
  }

  estado = PRONTO;
  desenhaPainelBase();
}

void abrirTelaZerarProducao() {
  estado = TELA_CONF;
  int sel = 1;
  bool sair = false;
  auto redesenhaProd = [&]() {
    tft.fillScreen(0x0000);
    for (int i = 0; i < 6; i++) tft.drawRoundRect(15 + i, 15 + i, 290 - (2 * i), 210 - (2 * i), 25, 0x07FF);
    tft.setTextSize(3);
    tft.setTextColor(0xFFFF);
    tft.setCursor(45, 60);
    tft.print("ZERAR SOLDAS?");
    tft.setTextSize(4);
    tft.setCursor(60, 140);
    tft.setTextColor(sel == 0 ? 0x07E0 : 0x3186);
    tft.print(sel == 0 ? ">SIM" : " SIM");
    tft.setCursor(190, 140);
    tft.setTextColor(sel == 1 ? 0xF800 : 0x3186);
    tft.print(sel == 1 ? ">NAO" : " NAO");
  };
  redesenhaProd();
  while (!sair) {
    esp_task_wdt_reset();
    int d = 0;
    if (lerEncoder(&d)) {
      sel = constrain(sel + d, 0, 1);
      redesenhaProd();
    }
    if (lerBotao()) {
      if (sel == 0) {
        soldasTotais = 0;
        prefs.begin("soldas", false);
        prefs.putUInt("count", 0);
        prefs.end();
        bipSuave();
        delay(100);
        bipSuave();
      }
      sair = true;
    }
  }
  estado = PRONTO;
  desenhaPainelBase();
}

// --- FUNÇÃO DE LEITURA DOS NTCs (CALIBRADA) ---
float lerTemperatura(int pino) {
    const float SERIESRESISTOR = 10000;
    const float NOMINAL_RESISTANCE = 10000;
    const float NOMINAL_TEMPERATURE = 25;
    const float BCOEFFICIENT = 3950;
    const float ADC_MAX = 4095.0;

    float reading = analogRead(pino);
    if (reading < 1) return -999; // Erro de leitura

    float resistance = SERIESRESISTOR / (ADC_MAX / reading - 1);
    float steinhart;
    steinhart = resistance / NOMINAL_RESISTANCE;      // (R/Ro)
    steinhart = log(steinhart);                         // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                         // 1/B * ln(R/Ro)
    steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                        // Inverter
    steinhart -= 273.15;                                // Converter para Celsius

    return steinhart;
}

void renderizarSensor(int pos, int pino) {
  if (estado != PRONTO) return;
  float t = lerTemperatura(pino);
  int yRef = 62 + (pos * 44);
  int xValor = 105;

  // Usamos o fundo no setTextColor para não precisar de fillRect no texto
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(3);
  tft.setCursor(xValor, yRef + 5);

  if (t < -900.0) {
    tft.print("OFF ");
  } else {
    tft.printf("%2.0f", t);
    tft.setTextSize(2);
    tft.print("c ");

    // A barra a gente limpa apenas o necessário
    int xBarra = xValor;
    int yBarra = yRef + 30;
    int larguraMax = 70;
    int progresso = map(constrain((int)t, 20, 80), 20, 80, 0, larguraMax);

    // Limpa o rastro da barra (o que sobra da barra anterior)
    tft.drawFastHLine(xBarra + progresso, yBarra, larguraMax - progresso, 0x0000);
    tft.drawFastHLine(xBarra + progresso, yBarra + 1, larguraMax - progresso, 0x0000);

    for (int i = 0; i < progresso; i++) {
      uint16_t cor;
      float p = (float)i / larguraMax;
      if (p < 0.33) cor = 0x07E0;       // Verde
      else if (p < 0.66) cor = 0xFFE0;  // Amarelo
      else cor = 0xF800;                // Vermelho
      tft.drawFastVLine(xBarra + i, yBarra, 4, cor);
    }
  }
}

void desenharCantoneirasHUD() {
  uint16_t corHUD = 0x3186;
  int l = 15;
  tft.drawFastHLine(0, 0, l, corHUD);
  tft.drawFastVLine(0, 0, l, corHUD);
  tft.drawLine(0, 0, 5, 5, corHUD);
  tft.drawFastHLine(320 - l, 0, l, corHUD);
  tft.drawFastVLine(319, 0, l, corHUD);
  tft.drawLine(319, 0, 314, 5, corHUD);
  tft.drawFastHLine(0, 239, l, corHUD);
  tft.drawFastVLine(0, 239 - l, l, corHUD);
  tft.drawLine(0, 239, 5, 234, corHUD);
  tft.drawFastHLine(320 - l, 239, l, corHUD);
  tft.drawFastVLine(319, 239 - l, l, corHUD);
  tft.drawLine(319, 239, 314, 234, corHUD);
}

void desenhaPainelBase() {
  tft.fillScreen(0x0000);
  desenharCantoneirasHUD();

  // --- BARRA DUPLA VERTICAL (ESTILO DUAL-RAIL) ---
  int xCentral = 180;          // Ponto central
  int espacamento = 4;         // Espaço entre as duas linhas
  int alturaLinha = 140;       // Tamanho definido anteriormente
  uint16_t corCinza = 0x8410;  // Cor cinza clarinho (RGB565)

  // Desenha a linha da esquerda
  tft.drawFastVLine(xCentral - (espacamento / 2) - 1, 55, alturaLinha, 0xFD20);

  // Desenha a NOVA linha central (cinza)
  tft.drawFastVLine(xCentral, 55, alturaLinha, corCinza);

  // Desenha a linha da direita
  tft.drawFastVLine(xCentral + (espacamento / 2) + 1, 55, alturaLinha, 0xFD20);

  uint16_t cores[] = { 0x07FF, 0xFFE0, 0xF81F };
  const char* labels[] = { "CABO", "TRAF", "SSR" };
  for (int i = 0; i < 3; i++) {
    int y = 58 + (i * 44);
    tft.fillRect(10, y, 3, 35, cores[i]);
    tft.drawFastHLine(10, y + 35, 155, 0x3186);
    tft.setTextSize(3);
    tft.setTextColor(cores[i]);
    tft.setCursor(20, y + 9);
    tft.print(labels[i]);
  }
  if (SD.exists("/imagem4.bmp")) reader.drawBMP("/imagem4.bmp", tft, 195, 56);
  if (SD.exists("/mascote.bmp")) reader.drawBMP("/mascote.bmp", tft, 158, 199);
  ultimaVelocidadeFan = -1;
  atualizarDinamicaCooler();
  tDados = 0;
}

// ============================================================================
// RELÓGIO DE TEMPO REAL E STATUS DE REDE
// ============================================================================

void atualizarRelogioESinal() {
  if (estado != PRONTO) return;
  static unsigned long tRel = 0;
  if (millis() - tRel < 1000) return;
  tRel = millis();

  corIndice = (corIndice + 1) % 6;
  uint16_t corT = coresTitulo[corIndice];

  // --- TÍTULO SP-01 GOLD ---
  tft.drawRoundRect(12, 8, 195, 38, 8, corT);
  tft.drawRoundRect(14, 10, 191, 34, 6, corT);
  tft.setTextSize(3);
  tft.setCursor(22, 16);
  tft.setTextColor(corT, 0x0000);
  tft.print("SP-01 GOLD");

  // --- RELÓGIO COM SEGUNDOS (Verde-Vivo) ---
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 10)) {
    // Minutos e Horas em Branco
    tft.setTextSize(2);
    tft.setTextColor(0xFFFF, 0x0000);
    tft.setCursor(218, 10);
    tft.printf("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

    // Segundos em Verde-Vivo (0x07E0)
    tft.setTextColor(0x07E0, 0x0000);
    tft.printf(":%02d", timeinfo.tm_sec);

    // Data abaixo (Amarelo)
    tft.setCursor(218, 28);
    tft.setTextColor(0xFFE0, 0x0000);
    tft.printf("%02d/%02d/%02d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year % 100);
  }

  // --- MOLDURAS DA COLUNA DIREITA ---
  int xCol = 210, wCol = 100, hCol = 28;

  // BLOCO WiFi (y=180)
  tft.drawRoundRect(xCol, 180, wCol, hCol, 6, 0xFFE0);

  if (WiFi.status() == WL_CONNECTED) {
    // Limpa área interna para garantir que o "OFF" sumiu
    tft.fillRect(xCol + 2, 182, wCol - 4, hCol - 4, 0x0000);
    tft.setTextSize(2);
    tft.setTextColor(0x07FF, 0x0000);
    tft.setCursor(xCol + 6, 187);
    tft.print("WiFi");

    int rssi = WiFi.RSSI();
    int bars = map(constrain(rssi, -100, -50), -100, -50, 1, 4);
    for (int b = 0; b < 4; b++) {
      uint16_t cB = (b < bars) ? 0x07E0 : 0x3186;
      tft.fillRect(xCol + 58 + (b * 9), 201 - (b * 5), 7, (b * 5) + 4, cB);
    }
  } else {
    // WiFi Desconectado: Exibe "WiFi OFF" em Vermelho
    tft.fillRect(xCol + 2, 182, wCol - 4, hCol - 4, 0x0000);  // Limpa as barrinhas
    tft.setTextSize(2);
    tft.setTextColor(0xF800, 0x0000);
    tft.setCursor(xCol + 10, 187);
    tft.print("WiFi OFF");
  }

  tft.drawRoundRect(xCol, 212, wCol, hCol, 6, 0xFFE0);  // Moldura Amarela

  // Limpa a área interna para a atualização
  tft.fillRect(xCol + 4, 214, wCol - 8, hCol - 4, 0x0000);

  // Leitura e Cálculo da Tensão
  int leituraRaw = analogRead(14);
  tensaoPAC = (leituraRaw / 4095.0) * 3.3 * FATOR_CALIBRACAO;

  tft.setTextSize(2);

  tft.setCursor(xCol + 6, 219);

  tft.setTextColor(0x07E0, 0x0000);  // Verde para o valor de tensão
  tft.printf("PAC %2.1fV", tensaoPAC);
}



void abrirTelaWiFi() {
  estado = TELA_WIFI;
  tft.fillScreen(0x0000);
  for (int i = 0; i < 6; i++) tft.drawRoundRect(15 + i, 15 + i, 290 - (2 * i), 210 - (2 * i), 25, 0x07FF);
  tft.setTextSize(2);
  tft.setTextColor(0x07FF);
  tft.setCursor(135, 40);
  tft.print("SSID");
  tft.setTextColor(0xFFFF);
  String ssid = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "OFFLINE";
  tft.setCursor(160 - (ssid.length() * 6), 65);
  tft.print(ssid);
  tft.setCursor(30, 125);
  tft.print("MAC: ");
  tft.print(WiFi.macAddress());
  tft.setCursor(30, 160);
  tft.print("IP:  ");
  tft.print(WiFi.localIP().toString());
  while (!lerBotao()) {
    esp_task_wdt_reset();
    delay(10);
  }
  estado = PRONTO;
  desenhaPainelBase();
}

// ============================================================================
// MEMÓRIA NÃO-VOLÁTIL (AUTO-SAVE)
// ============================================================================

bool lerBotao() {
  if (digitalRead(ENC_SW) == LOW) {
    unsigned long tAperto = millis();
    while (digitalRead(ENC_SW) == LOW) {
      if (millis() - tAperto > 2000) return true;  // Clique longo
      esp_task_wdt_reset();
      delay(5);
    }
    if (millis() - tAperto > 50) return true;  // Clique curto (debounce)
  }
  return false;
}

// --- FUNÇÃO DE SAÍDA DO SISTEMA ---
void executarSaidaSistema() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(20, 100);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Desligando Sistema...");
  delay(1000);
  ESP.restart();  // Reinicia o ESP32
}

// --- LÓGICA DE LEITURA DO ENCODER (PULSOS) ---
bool lerEncoder(int *delta) {
  static int ultimoEstadoCLK = HIGH;
  bool acionou = false;
  int estadoCLK = digitalRead(ENC_CLK);

  if (estadoCLK != ultimoEstadoCLK && estadoCLK == LOW) {
    if (digitalRead(ENC_DT) != estadoCLK) {
      *delta = 1;
    } else {
      *delta = -1;
    }
    acionou = true;
    bipEncoder();
  }
  ultimoEstadoCLK = estadoCLK;
  return acionou;
}

void bipEncoder() {
  // Se estiver usando o DFPlayer para o bip
  // somSerial.write(comando_bip); 
  // Ou se for um buzzer simples:
  // digitalWrite(PIN_BUZZER, HIGH); delay(10); digitalWrite(PIN_BUZZER, LOW);
}

void setup() {
  Serial.begin(115200);

  // Pinos e Saídas
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

  // --- LOGICA DE INICIALIZAÇÃO SP-01 GOLD ---
  if (SD.begin(SD_CS)) {
    estado = SPLASH1;
    if (SD.exists("/imagem1.bmp")) reader.drawBMP("/imagem1.bmp", tft, 0, 0);
    somGemini();
    tSplash = millis();
    carregarWiFiDoSD();  // Tenta SD primeiro
  } else {
    estado = PRONTO;
    desenhaPainelBase();
    WiFi.begin(WIFI_SSID, WIFI_PASS);  // Backup sem SD
  }

  configTime(-10800, 0, "pool.ntp.org", "time.google.com");

  esp_task_wdt_config_t twdt = { .timeout_ms = 30000, .idle_core_mask = 3, .trigger_panic = false };
  esp_task_wdt_init(&twdt);
  esp_task_wdt_add(NULL);
}
////////////////////////***************************************************************************************************************
void abrirTelaVolume() {
  estado = TELA_CONF;
  telaVolumeAtiva = true;

  tft.fillScreen(0x0000);

  // === MOLDURA LARGA PREMIUM (8 camadas) - Mantida em Verde ===
  for (int i = 0; i < 8; i++)
    tft.drawRoundRect(20 + i, 30 + i, 280 - (2 * i), 180 - (2 * i), 18, 0x07E0);

  // === IMAGEM 9 (ESQUERDA) ===
  if (SD.exists("/imagem9.bmp")) {
    reader.drawBMP("/imagem9.bmp", tft, 35, 45);
  }

  // === IMAGEM 8 (DIREITA) ===
  if (SD.exists("/imagem8.bmp")) {
    reader.drawBMP("/imagem8.bmp", tft, 225, 45);
  }

  // === TEXTO "VOLUME" CENTRALIZADO - Agora em LARANJA (0xFD20) ===
  tft.setTextSize(3);
  tft.setTextColor(0xFD20);  // <--- AQUI ESTÁ A MUDANÇA PARA LARANJA
  tft.setCursor(110, 60);
  tft.print("VOLUME");

  int larguraTotal = 200;
  int xBarra = 60;
  int yBarra = 125;
  int altura = 22;

  int volumeAnterior = -1;

  while (telaVolumeAtiva) {
    esp_task_wdt_reset();

    int d = 0;
    if (lerEncoder(&d)) {
      volumeSistema += (d * 5);
      volumeSistema = constrain(volumeSistema, 0, 100);
      bipEncoder(d);
    }

    if (volumeSistema != volumeAnterior) {
      // Limpa área da barra
      tft.fillRect(xBarra, yBarra, larguraTotal, altura, 0x0000);
      tft.drawRect(xBarra, yBarra, larguraTotal, altura, 0xFFFF);

      int larguraVolume = map(volumeSistema, 0, 100, 0, larguraTotal - 2);

      for (int i = 0; i < larguraVolume; i++) {
        uint16_t cor = obterCorDegrade(i, larguraTotal);
        tft.drawFastVLine(xBarra + 1 + i, yBarra + 1, altura - 2, cor);
      }

      // Percentual centralizado abaixo da barra
      tft.fillRect(120, 160, 80, 20, 0x0000);
      tft.setTextSize(2);
      tft.setTextColor(0xFFFF);
      tft.setCursor(135, 160);
      tft.printf("%d%%", volumeSistema);

      volumeAnterior = volumeSistema;
    }

    if (lerBotao()) {
      prefs.begin("soldas", false);
      prefs.putInt("volume", volumeSistema);
      prefs.end();
      telaVolumeAtiva = false;
    }
    delay(10);
  }

  estado = PRONTO;
  desenhaPainelBase();
}

// ============================================================================
// MODO BYPASS (ACIONAMENTO DIRETO)
// ============================================================================

void abrirTelaBypass() {
  estado = TELA_CONF;
  int sel = 1;  // Começa no "NÃO" por segurança
  bool sair = false;

  auto redesenhaBypass = [&]() {
    tft.fillScreen(0x0000);  // Limpa a tela inteira para garantir um desenho limpo

    // MOLDURA DE RISCO - 8 Camadas em Vermelho Vivo
    for (int i = 0; i < 8; i++) tft.drawRoundRect(15 + i, 15 + i, 290 - (2 * i), 210 - (2 * i), 25, 0xF800);

    // 1. TÍTULO: "MODO BYPASS?"
    tft.setTextSize(3);
    tft.setTextColor(0xF800);  // Texto em vermelho
    tft.setCursor(55, 40);
    tft.print("MODO BYPASS?");

    // --- NOVO: SUBLINHADO LARANJA-VIVO ---

    tft.fillRect(55, 66, 210, 3, 0xFD20);

    // 2. IMAGEM CENTRALIZADA (imagem11.bmp - 72x72 pixels)
    // Primeiro, limpamos a área central para garantir que o texto "ATENCAO" antigo suma
    tft.fillRect(100, 76, 120, 80, 0x0000);

    if (SD.exists("/imagem11.bmp")) {
      reader.drawBMP("/imagem11.bmp", tft, 124, 76);
    } else {
      // Caso a imagem falhe, exibe o aviso centralizado
      tft.setTextSize(2);
      tft.setTextColor(0xFFFF);
      tft.setCursor(115, 110);
      tft.print("ATENCAO");
    }

    tft.setTextSize(4);

    // Opção SIM
    tft.setCursor(60, 180);
    tft.setTextColor(sel == 0 ? 0xF800 : 0x3186);  // Vermelho se selecionado
    tft.print(sel == 0 ? ">SIM" : " SIM");

    // Opção NÃO
    tft.setCursor(190, 180);
    tft.setTextColor(sel == 1 ? 0x07E0 : 0x3186);  // Verde se selecionado
    tft.print(sel == 1 ? ">NAO" : " NAO");
  };

  redesenhaBypass();

  while (!sair) {
    esp_task_wdt_reset();
    int d = 0;
    if (lerEncoder(&d)) {
      sel = constrain(sel + d, 0, 1);
      bipEncoder(d);
      redesenhaBypass();
    }

    if (lerBotao()) {
      if (sel == 0) {
        digitalWrite(PIN_BYPASS, HIGH);
        bypassAtivo = true;
        // Som de alerta longo para indicar perigo/ativação
        tocarSom(1500, 500);
      } else {
        digitalWrite(PIN_BYPASS, LOW);
        bypassAtivo = false;
      }
      sair = true;
    }
    delay(10);
  }
  estado = PRONTO;
  desenhaPainelBase();
}

void atualizarLeituraPAC() {
  long somaRaw = 0;
  for (int i = 0; i < 10; i++) {
    somaRaw += analogRead(PIN_PAC_READ);
    delay(2);
  }
  float leituraRaw = somaRaw / 10.0;

  // Cálculo da tensão real (Fator 2.51)
  tensaoPAC = (leituraRaw * 3.3 / 4095.0) * 2.51;

  // Porcentagem para Pack 2S (6.4V a 8.4V)
  float p = (tensaoPAC - 6.4) / (8.4 - 6.4) * 100.0;
  porcentagemPAC = constrain(p, 0, 100);
}

void loop() {
  esp_task_wdt_reset();  // Avisa o sistema que está tudo bem

  // --- 1. GESTÃO DE REDE E HORA (NON-BLOCKING) ---
  static unsigned long tRede = 0;
  if (estado != SOLDANDO && (millis() - tRede > 10000)) {
    tRede = millis();
    if (WiFi.status() == WL_CONNECTED) {
      ArduinoOTA.handle();
    } else {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
    }
  }

  // --- 2. LÓGICA DE SPLASH (TELAS INICIAIS) ---
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
    return;  // Sai do loop para não processar menu durante o Splash
  }

  // --- 3. PROCESSO DE SOLDA (A PARTE MAIS IMPORTANTE) ---
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
    return;  // Durante a solda, não lê encoder nem desenha relógio
  }

  // --- 4. INTERFACE E SENSORES (SÓ SE ESTIVER EM "PRONTO") ---
  if (estado == PRONTO) {
    atualizarRelogioESinal();

    // Atualiza sensores e fan a cada INTERVALO definido
    if (millis() - tDados > INTERVALO_ATUALIZACAO_PAINEL) {
      tDados = millis();
      renderizarSensor(0, NTC_CABO);
      renderizarSensor(1, NTC_TRAFO);
      renderizarSensor(2, NTC_SSR);
      atualizarLeituraPAC();
      logicaAnimacaoCooler();

      // Desenha o seletor de material/configuração no rodapé
      tft.fillRect(10, 198, 140, 25, 0x0000);
      tft.setCursor(10, 198);
      tft.setTextSize(2);

      // Cores dinâmicas para o menu
      uint16_t corMenu = ajustandoCustom ? 0xFFE0 : 0x07FF;
      tft.setTextColor(corMenu);

      if (materialIndex == 6) tft.printf("> Cust:%d%%", customPercent);
      else if (materialIndex == 10) tft.printf("> VOL:%d", volumeAtual);
      else tft.printf("> %s", materiais[materialIndex].nome);
    }

    // --- 5. ENCODER E BOTÃO ---
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
      tDados = 0;  // Força atualização da tela
    }

    if (lerBotao()) {
      if (materialIndex == 6 || materialIndex == 10) {
        ajustandoCustom = !ajustandoCustom;
        bipSuave();
      }
      if (lerBotao()) {
        if (materialIndex == 6 || materialIndex == 10) {
          ajustandoCustom = !ajustandoCustom;
          bipSuave();
        } else if (materialIndex == 7) {
          // --- LOGICA DE CLIQUE DIFERENCIADO ---
          unsigned long tInicioAperto = millis();
          bool longo = false;
          while (digitalRead(ENC_SW) == LOW) {
            if (millis() - tInicioAperto > 2000) {
              longo = true;
              break;
            }
            delay(10);
          }

          if (longo) iniciarWPS();  // Clique Longo = Parear
          else abrirTelaWiFi();     // Clique Curto = Ver Info
        } else if (materialIndex == 8) abrirTelaZerarProducao();
        else if (materialIndex == 9) abrirTelaAtuliz();
        else if (materialIndex == 11) abrirTelaBypass();
        else if (materialIndex == 12) executarSaidaSistema();
        tDados = 0;
      } else if (materialIndex == 8) abrirTelaZerarProducao();
      else if (materialIndex == 9) abrirTelaAtuliz();
      else if (materialIndex == 11) abrirTelaBypass();
      else if (materialIndex == 12) executarSaidaSistema();
      tDados = 0;
    }

    // Gatilho da Solda pelo Pedal (Prioridade Máxima)
    if (digitalRead(PIN_CANETA) == LOW && materialIndex < 7) {
      if (digitalRead(PIN_PEDAL) == LOW && pedalLiberado) {
        pedalLiberado = false;
        estado = SOLDANDO;
        faseSolda = 1;
        tSoldaInicio = millis();
        
        // 1º DISPARA O SSR (Tempo Real)
        digitalWrite(PIN_SSR, HIGH); 
        
        // 2º DEPOIS desenha na tela (O processo de solda já iniciou no hardware)
        if (SD.exists("/soldando.bmp")) {
             // Otimização: Só desenha se não estiver no meio do pulso crítico
             reader.drawBMP("/soldando.bmp", tft, 0, 0);
        }
      }
    }

  }

  if (digitalRead(PIN_PEDAL) == HIGH) { pedalLiberado = true; }

  // --- 6. AUTO-SAVE (CADA 1 MINUTO) ---
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

//       END  //
////////////////////////////////////////////////////////////  NÓS ACREDITAMOS EM DEUS!!!!  ////////////////////////////////////////////////

