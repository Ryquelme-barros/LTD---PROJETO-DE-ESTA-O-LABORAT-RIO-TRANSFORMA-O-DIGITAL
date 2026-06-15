#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"

// ======================================================
// CONFIGURAÇÃO DO WIFI E THINGSPEAK
// ======================================================
const char* ssid = "AGRI.COORDENACAO";
const char* password = "cordenacao";

// Escreva aqui a sua chave:
String apiKey = "O3NM9UKELEGFLA4G"; 

// ======================================================
// MAPEAMENTO DE PINOS E CONFIGURAÇÕES DO SENSOR
// ======================================================
const int ledVermelho = 25;  // GPIO 25 (Seco ou Alagado)
const int ledVerde = 26;     // GPIO 26 (Ideal / Úmido)
const int ledAmarelo = 27;   // GPIO 27 (Muito Úmido)
const int rele = 33;

const int pinoSolo = 34;     // GPIO 34 (Sensor de Solo)
#define DHTPIN 5             // GPIO 5 (Sensor DHT11)
#define DHTTYPE DHT11   

// Inicializa o sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Variáveis do solo
int valorSoloAnalogico = 0;
int porcentagemUmidadeSolo = 0;

void setup() {
  // Inicializa a comunicação serial
  Serial.begin(115200);
  delay(10); 

  // Configura os pinos dos LEDs como saídas
  pinMode(ledVermelho, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(rele, OUTPUT);

  // Inicializa o sensor DHT11
  dht.begin();

  // Teste pisca-alerta rápido dos LEDs ao ligar
  digitalWrite(ledVermelho, HIGH);
  digitalWrite(ledVerde, HIGH);
  digitalWrite(ledAmarelo, HIGH);
  delay(500);
  digitalWrite(ledVermelho, LOW);
  digitalWrite(ledVerde, LOW);
  digitalWrite(ledAmarelo, LOW);

  // ======================================================
  // CONECTAR WIFI
  // ======================================================
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado!");

  Serial.println("==================================");
  Serial.println("         SISTEMA INICIADO         ");
  Serial.println("==================================");
}

void loop() {
  // --- 1. LEITURA E CONVERSÃO DO SENSOR DE SOLO ---
  valorSoloAnalogico = analogRead(pinoSolo);

  // Mapeia o valor de 0-4095 para 100%-0% (Invertido: 4095 = 0% de umidade)
  porcentagemUmidadeSolo = map(valorSoloAnalogico, 4095, 0, 0, 100);
  
  // Garante que o valor fique estritamente entre 0 e 100%
  porcentagemUmidadeSolo = constrain(porcentagemUmidadeSolo, 0, 100);

  Serial.print(F("Leitura Analógica: "));
  Serial.print(valorSoloAnalogico);
  Serial.print(F(" | Umidade do Solo: "));
  Serial.print(porcentagemUmidadeSolo);
  Serial.print(F("%"));

  // --- LÓGICA DAS SITUAÇÕES DO SOLO ---
  
  // 🌵 Solo Seco: 0% a 40%
  if (porcentagemUmidadeSolo <= 40) {
    Serial.println(F(" -> [STATUS: SOLO SECO]"));
    digitalWrite(ledVermelho, HIGH); // Alerta de seco
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarelo, LOW);
    digitalWrite(rele, HIGH);
  }
  // 🌱 Solo Úmido (Ideal): 41% a 75%
  else if (porcentagemUmidadeSolo >= 41 && porcentagemUmidadeSolo <= 75) {
    Serial.println(F(" -> [STATUS: SOLO ÚMIDO (IDEAL)]"));
    digitalWrite(ledVermelho, LOW);
    digitalWrite(ledVerde, HIGH);    // Verde para ideal
    digitalWrite(ledAmarelo, LOW);
    digitalWrite(rele, LOW);
  }
  // 💧 Solo Muito Úmido: 76% a 85%
  else if (porcentagemUmidadeSolo >= 76 && porcentagemUmidadeSolo <= 85) {
    Serial.println(F(" -> [STATUS: SOLO MUITO ÚMIDO]"));
    digitalWrite(ledVermelho, LOW);
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarelo, HIGH);   // Amarelo para atenção
    digitalWrite(rele, LOW);
  }
  // 🚨 Solo Alagado: Acima de 85%
  else {
    Serial.println(F(" -> [STATUS: ALAGADO]"));
    // Pisca ou mantém o vermelho/amarelo para indicar o perigo de alagamento
    digitalWrite(ledVermelho, HIGH); 
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarelo, HIGH);
    digitalWrite(rele, LOW);
  }

  // --- 2. LEITURA DO SENSOR DHT11 (AR) ---
  float umidadeAr = dht.readHumidity();
  float temperatura = dht.readTemperature();

  if (isnan(umidadeAr) || isnan(temperatura)) {
    Serial.println(F("Erro: Falha ao ler o sensor DHT11!"));
    temperatura = 0; 
  } else {
    Serial.print(F("Temperatura do Ar: "));
    Serial.print(temperatura);
    Serial.print(F("°C  |  Umidade do Ar: "));
    Serial.print(umidadeAr);
    Serial.println(F("%"));
  }

  // ======================================================
  // ENVIAR DADOS PARA O THINGSPEAK
  // ======================================================
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Agora enviamos a PORCENTAGEM calculada para o Field 1
    String url = "http://api.thingspeak.com/update?api_key=" + apiKey +
                 "&field1=" + String(porcentagemUmidadeSolo) +
                 "&field2=" + String(temperatura);

    http.begin(url);

    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.println("Dados enviados ao ThingSpeak!");
    } else {
      Serial.println("Erro ao enviar dados.");
    }

    http.end();
  } else {
    Serial.println("Wi-Fi desconectado. Não foi possível enviar para o ThingSpeak.");
  }

  Serial.println("----------------------------------");
  delay(20000); // 20 segundos de intervalo
}