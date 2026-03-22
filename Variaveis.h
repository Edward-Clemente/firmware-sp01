#ifndef VAR_H
#define VAR_H

// --- OBJETOS GLOBAIS ---
// Criamos os objetos aqui para que todas as abas os conheçam
SdFat SD;
Adafruit_ImageReader reader(SD);
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
Preferences prefs;
HardwareSerial somSerial(2);
DFRobotDFPlayerMini myDFPlayer;
esp_wps_config_t configWps;

// --- ESTADOS E MATERIAIS ---
enum Estado { SPLASH1, SPLASH2, SPLASH3, PRONTO, SOLDANDO, TELA_WIFI, TELA_PROD, TELA_CONF, TELA_UPDATING };
Estado estado = SPLASH1;

struct Material {
  const char* nome;
  uint16_t p1, p2, intervalo;
};

Material materiais[] = {
  { "Ni 0.10", 10, 40, 100 }, { "Ni 0.12", 15, 60, 150 }, { "Ni 0.15", 20, 90, 200 }, 
  { "Ni 0.18", 25, 130, 250 }, { "ACO NIQ", 30, 180, 300 }, { "COBRE", 50, 400, 400 }, 
  { "CUST", 15, 60, 200 }, { "WIFI", 0, 0, 0 }, { "PRODUCAO", 0, 0, 0 }, 
  { "ATUALIZ.", 0, 0, 0 }, { "SOM: ON", 0, 0, 0 }, { "BYPASS", 0, 0, 0 }, { "SAIR", 0, 0, 0 }
};

// --- VARIÁVEIS DE OPERAÇÃO ---
const char* VERSAO_ATUAL = "1.1.4";
const char* WIFI_SSID = "Monte Alto_EXT";
const char* WIFI_PASS = "21216666Aa";
const String updateUrl = "https://raw.githubusercontent.com/Edward-Clemente/firmware-sp01/main/up.bin";
const String IP_LOCAL_PC = "http://192.168.137.1/atualizar/up.bin";

int materialIndex = 0, customPercent = 15, volumeAtual = 25, volumeSistema = 70;
uint32_t soldasTotais = 0, minutosTotais = 0;
float tensaoPAC = 0.0, tempSSRGlobal = 0, tempTrafGlobal = 0, tempCaboGlobal = 0;
int velocidadeFanAtual = 10, ultimaVelocidadeFan = -1, fanState = 10, etapaFan = 10, porcentagemPAC = 0;
bool subindoFan = true, segurandoNoMaximo = false, bypassAtivo = false;
bool telaVolumeAtiva = false, ajustandoCustom = false, pedalLiberado = true, bipHabilitado = true;
unsigned long tSplash = 0, tDados = 0, tSoldaInicio = 0, tempoUltimoPassoFan = 0, tempoMaximoFan = 0;
int faseSolda = 0, corIndice = 0;
uint16_t coresTitulo[] = { 0xFFFF, 0x07FF, 0x07E0, 0xF81F, 0xFD20, 0x07E0 };
const int pinoSair = 0;

// --- PROTÓTIPOS (AVISOS PRÉVIOS) ---
void tocarSom(int frequencia, int duracao);
void somGemini();
void somHarmonicoA();
void somHarmonicoB();
void bipSuave();
void bipEncoder(int dir);
bool lerBotao();
bool lerEncoder(int *delta);
void carregarWiFiDoSD();
void iniciarWPS();
float lerTemperatura(int pino);
void renderizarSensor(int pos, int pino);
void logicaAnimacaoCooler();
void desenhaPainelBase();
void atualizarRelogioESinal();
void atualizarLeituraPAC();
void abrirTelaWiFi();
void abrirTelaAtuliz();
void abrirTelaZerarProducao();
void executarSaidaSistema();
void executarUpdate(String url);
uint16_t obterCorDegrade(int i, int total);

#endif