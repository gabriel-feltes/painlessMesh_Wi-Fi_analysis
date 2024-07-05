#define INTERNET "SSID" // DYNAMIC INTERNET CONECTIVITY FOR DEBUGGING NODE INDIRECT CONNECTIVITY

#include <SFE_BMP180.h>     // Biblioteca para sensor BMP180
#include <DHT.h>
#include <Wire.h>           // Biblioteca para comunicação I2C
#include "ESP8266WiFi.h"
#include <painlessMesh.h>   // Biblioteca para criar a rede mesh

const int RSSI_MAX = -50;    // define maximum strength of signal in dBm
const int RSSI_MIN = -100;   // define minimum strength of signal in dBm

const int displayEnc = 1;    // set to 1 to display Encryption or 0 not to display

// Function prototypes
int dBmtoPercentage(int dBm);
String encType(int id);

struct NetworkInfo {
  String ssid;
  int channel;
  int rssi;
  int percentage;
  String mac;
  bool hidden;
  String encryption;
};

NetworkInfo networks[50];    // Assuming maximum of 50 networks, adjust as necessary

int meshNetworkRSSI = 0;
int meshNetworkChannel = 0;
int internetRSSI = 0;
int internetChannel = 0;

void getNetworkInfo() {
  String output = "";
  output += "Wifi scan started\n";
  int n = WiFi.scanNetworks();
  output += "Wifi scan ended\n";

  if (n == 0) {
    output += "No networks found\n";
  } else {
    output += String(n) + " networks found\n";

    for (int i = 0; i < n; ++i) {
      networks[i].ssid = WiFi.SSID(i);
      networks[i].channel = WiFi.channel(i);
      networks[i].rssi = WiFi.RSSI(i);
      networks[i].percentage = dBmtoPercentage(WiFi.RSSI(i));
      networks[i].mac = WiFi.BSSIDstr(i);
      networks[i].hidden = WiFi.isHidden(i);
      if (displayEnc) {
        networks[i].encryption = encType(i);
      }

      // Check for meshNetwork and INTERNET
      if (networks[i].ssid == "meshNetwork") {
        meshNetworkRSSI = networks[i].percentage;
        meshNetworkChannel = networks[i].channel;
      } else if (networks[i].ssid == INTERNET) {
        internetRSSI = networks[i].percentage;
        internetChannel = networks[i].channel;
      }

      delay(10);
    }

    // Sort networks based on percentage signal strength
    for (int i = 0; i < n - 1; i++) {
      for (int j = i + 1; j < n; j++) {
        if (networks[i].percentage < networks[j].percentage) {
          NetworkInfo temp = networks[i];
          networks[i] = networks[j];
          networks[j] = temp;
        }
      }
    }

    // Construct the output string with sorted network information
    for (int i = 0; i < n; ++i) {
      output += String(i + 1) + ") ";
      output += networks[i].ssid + " ch:";
      output += String(networks[i].channel) + " ";
      output += String(networks[i].rssi) + "dBm (";
      output += String(networks[i].percentage) + "%)";

      output += " MAC:";
      output += networks[i].mac;

      if (networks[i].hidden) {
        output += " <<Hidden>>";
      }
      if (displayEnc) {
        output += " Encryption:";
        output += networks[i].encryption;
      }

      output += "\n";
    }
  }

  output += "\n";
  delay(5000);
  WiFi.scanDelete();
  Serial.println(output);
}

String encType(int id) {
  String type;
  switch (WiFi.encryptionType(id)) {
    case ENC_TYPE_WEP:
      type = " WEP";
      break;
    case ENC_TYPE_TKIP:
      type = " WPA / PSK";
      break;
    case ENC_TYPE_CCMP:
      type = " WPA2 / PSK";
      break;
    case ENC_TYPE_AUTO:
      type = " WPA / WPA2 / PSK";
      break;
    case ENC_TYPE_NONE:
      type = " <<OPEN>>";
      break;
    default:
      type = " Unknown";
      break;
  }
  return type;
}

int dBmtoPercentage(int dBm) {
  int quality;
  if (dBm <= RSSI_MIN) {
    quality = 0;
  } else if (dBm >= RSSI_MAX) {
    quality = 100;
  } else {
    quality = 2 * (dBm + 100);
  }
  return quality;
}

// Temperatura e umidade
#define DHTPIN D3             // Pino D3 será responsável pela leitura do DHT11
#define DHTTYPE DHT11         // Define o DHT11 como o sensor a ser utilizado pela biblioteca <DHT.h>
DHT dht(DHTPIN, DHTTYPE);     // Inicializando o objeto dht do tipo DHT passando como parâmetro o pino (DHTPIN) e o tipo do sensor (DHTTYPE)
float u = 0.0;                // Variável responsável por armazenar a umidade lida pelo DHT11
float t = 0.0;                // Variável responsável por armazenar a temperatura lida pelo DHT11

// Pressão atmosférica
SFE_BMP180 sensorP;           // Define objeto sensorP na classe SFE_BMP180 da biblioteca
#define ALTITUDE 27           // Altitude da UFSC, em metros
char status;                  // Variável auxiliar para verificação do resultado
double temperatura;           // Variável para armazenar a temperatura
double pressao_abs;           // Variável para armazenar a pressão absoluta
double pressao_relativa = 0.0;// Variável para armazenar a pressão relativa

int led = D8;                 // Atribui a porta digital D1 (GPIO5) a variável led
int ldr = A0;                 // Atribui A0 a variável ldr
int valorldr = 0;             // Declara a variável valorldr como inteiro

// Configurações da rede mesh
painlessMesh mesh;
#define MESH_PREFIX "meshNetwork"
#define MESH_PASSWORD "meshPassword"
#define MESH_PORT 5555

// Função para leitura da temperatura e umidade - Sensor DHT11
void sensorDHT() {
  u = dht.readHumidity();     // Realiza a leitura da umidade
  t = dht.readTemperature();  // Realiza a leitura da temperatura
  if (isnan(u) || isnan(t)) {
    Serial.println("Falha na leitura do sensor DHT11!");
  } else {
    Serial.print("Umidade: ");
    Serial.println(u);          // Imprime na serial a umidade
    Serial.print("Temperatura: ");
    Serial.println(t);          // Imprime na serial a temperatura
  }
}

// Função para leitura da pressão absoluta e relativa - Sensor BMP180
void Pressao() {
  status = sensorP.startTemperature(); // Inicializa a leitura da temperatura
  if (status != 0) {                   // Se status for diferente de zero (sem erro de leitura)
    delay(status);                     // Realiza uma pequena pausa para que a leitura seja finalizada
    status = sensorP.getTemperature(temperatura); // Armazena o valor da temperatura na variável temperatura
    if (status != 0) {                 // Se status for diferente de zero (sem erro de leitura)
      // Leitura da Pressão Absoluta
      status = sensorP.startPressure(3); // Inicializa a leitura
      if (status != 0) {                 // Se status for diferente de zero (sem erro de leitura)
        delay(status);                   // Realiza uma pequena pausa para que a leitura seja finalizada
        status = sensorP.getPressure(pressao_abs, temperatura); // Atribui o valor medido de pressão à variável pressao, em função da variável temperatura
        Serial.print("Pressão absoluta: ");
        Serial.println(pressao_abs, 1);  // Imprime na serial a pressão absoluta
        if (status != 0) {               // Se status for diferente de zero (sem erro de leitura)
          pressao_relativa = sensorP.sealevel(pressao_abs, ALTITUDE); // Atribui o valor medido de pressão relativa à variavel pressao_relativa, em função da ALTITUDE
          Serial.print("Pressão relativa: ");
          Serial.println(pressao_relativa, 1); // Imprime na serial a pressão relativa
        }
      }
    }
  }
}

// Função para controle da luz com base na leitura do LDR
void luz() {
  valorldr = analogRead(ldr);        // Lê o valor do sensor ldr e armazena na variável valorldr
  Serial.print("Valor lido pelo LDR = "); // Imprime na serial a mensagem "Valor lido pelo LDR"
  Serial.println(valorldr);          // Imprime na serial os dados de valorldr
  if (valorldr < 500)                // Se o valor de valorldr for menor que 500:
    digitalWrite(led, HIGH);         // Coloca led em alto para acioná-lo
  else
    digitalWrite(led, LOW);          // Coloca led em baixo para que o mesmo desligue ou permaneça desligado
}

// Função de callback para a rede mesh ao receber mensagem
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Mensagem recebida de %u: %s\n", from, msg.c_str());
}

// Callback para nova conexão estabelecida na rede mesh
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("Nova conexão estabelecida, nodeId = %u\n", nodeId);
}

// Callback para alterações nas conexões da rede mesh
void changedConnectionCallback() {
  Serial.printf("Conexões alteradas\n");
}

// Callback para ajuste de tempo nos nós da rede mesh
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Horário ajustado. Offset = %d\n", offset);
}

// Configuração inicial do dispositivo
void setup() {
  Serial.begin(74880);              // Inicializa a comunicação serial
  pinMode(led, OUTPUT);             // Define led (pino digital 10) como saída
  pinMode(ldr, INPUT);              // Define ldr (pino analógico A0) como entrada
  sensorP.begin();                  // Inicializa o sensor de pressão atmosférica
  dht.begin();                      // Inicializa o sensor DHT11

  // Configuração inicial da rede mesh
  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}

unsigned long lastWifiScanTime = 0;
const unsigned long wifiScanInterval = 30000; // Scan every 5 minutes (adjust as needed)

// Loop principal do dispositivo
void loop() {
  Serial.println("====== Coleta de dados ======");
  luz();                // Realiza a leitura do LDR e controla a luz
  sensorDHT();          // Realiza a leitura do sensor DHT11 (umidade e temperatura)
  Pressao();            // Realiza a leitura do sensor BMP180 (pressão atmosférica)

  // Perform WiFi scan every 5 minutes
  unsigned long currentMillis = millis();
  if (currentMillis - lastWifiScanTime >= wifiScanInterval) {
    lastWifiScanTime = currentMillis;
    getNetworkInfo();
  }

  // Atualizações na rede mesh
  mesh.update();

  // Envio de dados pela rede mesh
  String msg = String(pressao_relativa) + "," +
               String(pressao_abs) + "," +
               String(t) + "," +
               String(u) + "," +
               String(valorldr) + "," +
               String(meshNetworkRSSI) + "," +
               String(meshNetworkChannel) + "," +
               String(internetRSSI) + "," +
               String(internetChannel);

  mesh.sendBroadcast(msg);

  delay(2000);          // Intervalo de 2 segundos antes da próxima leitura
}
