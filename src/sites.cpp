#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include "credenciais.h"

#include <WebServer.h>
#include <uri/UriBraces.h>

#include <LittleFS.h>
#include <ArduinoJson.h>

#include <MQTT.h>

// LittleFS [pendente] -> arquivos html dentro do ESP32
// MQTT [confirmações]
// MQTT [envio] -> pode enviar mais de uma vez o login (1)
// qual é a porta para conexão MQTT com TLS

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);
WebServer servidor(80);

String nomeRede = WIFI_NOME;
String senhaRede = WIFI_SENHA;
String nomeUsuario;
String cpfUsuario;

String lerRespostaSerial() {
  String resposta;
  
  while (true) {
    if (!Serial.available()) {
      delay(5);
      continue;
    }
    
    char caractere = Serial.read();
    if (caractere == '\n') {
      return resposta;
    }
    
    // '\r' pode aparecer em alguns retrnos [precaução]
    if (caractere != '\r') {
      resposta += caractere;
    }
  }
}
void confirmarRedeWiFi() {
  Serial.println("\nRede Atual: " + nomeRede);
  Serial.println("A rede segue igual? (s/n)");
  
  String resposta;
  do {
    resposta = lerRespostaSerial();
    resposta.trim();
    resposta.toLowerCase();
  } while (resposta != "s" && resposta != "n");
  
  if (resposta == "n") {
    do {
      Serial.println("Digite o nome da nova rede:");
      nomeRede = lerRespostaSerial();
    } while (nomeRede.length() == 0);
    
    Serial.println("Digite a senha:");
    senhaRede = lerRespostaSerial();
  }
}
void ArquivoHTML() {
  if (LittleFS.exists("/login.html")) {
    return;
  }
  
  Serial.println("ERRO: login.html nao esta na memoria do ESP32.");
  Serial.println("Envie a pasta data para o LittleFS e reinicie a placa.");
  while (true) {
    delay(100); 
  }
}



void pagina1 () {
  File arquivo = LittleFS.open("/login.html", "r");
  if (!arquivo) {
    servidor.send(500, "text/html", "Erro no HTML");
    return;
  }
  String html = arquivo.readString();
  arquivo.close();
  
  servidor.send(200, "text/html", html);
}
void pagina2 () {
  servidor.send(200, "text/html", "Bem-vindo!");
}
void pagina0 () {
  servidor.send(200, "text/html", "Obrigado!!!");
}
void tratarDados() {
  JsonDocument dados;
  String textoJson;
  
  nomeUsuario = servidor.arg("nome");
  cpfUsuario = servidor.arg("cpf");
  dados["usuario"] = nomeUsuario;
  dados["cpf"] = cpfUsuario;
  
  serializeJson(dados, textoJson);
  mqtt.publish("totem/login", textoJson, false, 1);
  
  servidor.sendHeader("Location", "/login_realizado");
  servidor.send(303);
}



void reconectarWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(nomeRede.c_str(), senhaRede.c_str());
    Serial.print("Conectando ao WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }
    Serial.print("conectado!\nEndereco IP: ");
    Serial.println(WiFi.localIP());
  }
}
void reconectarMQTT() {
  if (!mqtt.connected()) {
    Serial.print("Conectando MQTT...");
    while(!mqtt.connected()) {
      mqtt.connect(MQTT_CLIENTE_ID, MQTT_USUARIO, MQTT_SENHA);
      Serial.print(".");
      delay(1000);
    }
    Serial.println(" conectado!");
  }
}

void setup() {
  Serial.begin(115200); delay(500);
  confirmarRedeWiFi();
  
  if (!LittleFS.begin()) {
    Serial.println("LittleFS falhou!");
    while (true) {};
  }
  
  ArquivoHTML();
  
  conexaoSegura.setCACert(certificado1);
  mqtt.begin("mqtt.janks.dev.br", portaTLS, conexaoSegura); 
  mqtt.setKeepAlive(10);
  
  servidor.on("/login", HTTP_GET, pagina1);
  servidor.on("/login", HTTP_POST, tratarDados);
  servidor.on("/login_realizado", HTTP_GET, pagina0);
  servidor.on("/loja", HTTP_GET, pagina2);
  servidor.begin();
  
  reconectarWiFi();
  reconectarMQTT();
}

void loop() {
  reconectarWiFi();
  reconectarMQTT();
  mqtt.loop();
  
  servidor.handleClient();
}
