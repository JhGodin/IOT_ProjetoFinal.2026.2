#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "credenciais/certificados.h"
#include "credenciais/credenciais.h"

#include <WebServer.h>
#include <uri/UriBraces.h>

#include <LittleFS.h>
#include <ArduinoJson.h>

#include <MQTT.h>

#include <SPI.h>
#include <MFRC522.h>

#include "credenciais/credenciais.h"

MFRC522 rfid(46, 17);
MFRC522::MIFARE_Key chaveA = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

// LittleFS [pendente] -> arquivos html dentro do ESP32

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);
WebServer servidor(80);




String lerRFID()
{
  String id = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    if (i > 0)
    {
      id += " ";
    }
    if (rfid.uid.uidByte[i] < 0x10)
    {
      id += "0";
    }
    id += String(rfid.uid.uidByte[i], HEX);
  }
  id.toUpperCase();
  return id;
}


//corrigir
void recebeuMensagem(String topico, String conteudo)
{
  Serial.println(topico + ": " + conteudo);
  if (topico == "totem/canal_id")
  {
  }
}
void pagina2()
{
  File arquivo = LittleFS.open("/cardapio.html", "r");
  if (!arquivo)
  {
    servidor.send(500, "text/html", "Erro no HTML [cardapio]");
    return;
  }
  String html = arquivo.readString();
  arquivo.close();

  servidor.send(200, "text/html", html);
}


void reconectarWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(WIFI_NOME, WIFI_SENHA);
    Serial.print("Conectando ao WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(1000);
    }
    Serial.print("conectado!\nEndereco IP: ");
    Serial.println(WiFi.localIP());
  }
}
void reconectarMQTT()
{
  if (!mqtt.connected())
  {
    Serial.print("Conectando MQTT...");
    while (!mqtt.connected())
    {
      mqtt.connect(MQTT_CLIENTE_ID, MQTT_USUARIO, MQTT_SENHA);
      Serial.print(".");
      delay(1000);
    }
    Serial.println(" conectado!");
  }
}




void setup()
{
  Serial.begin(115200);
  delay(500);
  
  // RFid
  SPI.begin();
  rfid.PCD_Init();

  // Arq. ESP
  if (!LittleFS.begin())
  {
    Serial.println("LittleFS falhou!");
    while (true)
    {
    };
  }
  if (!LittleFS.exists("/cardapio.html"))
  {
    Serial.println("ERRO: cardapio.html nao esta na memoria do ESP32.");
    Serial.println("Envie a pasta data para o LittleFS e reinicie a placa.");
    while (true)
    {
      delay(100);
    }
  }

  // MQTT
  conexaoSegura.setCACert(certificado1);
  mqtt.begin("mqtt.janks.dev.br", 1883, conexaoSegura);
  mqtt.setKeepAlive(10);

  // WebServer
  servidor.on("/loja", HTTP_GET, pagina2);
  servidor.on("/loja", HTTP_POST, tratarDados);

  mqtt.onMessage(recebeuMensagem);
  servidor.begin();

  reconectarWiFi();
  reconectarMQTT();
}
void loop()
{
  reconectarWiFi();
  reconectarMQTT();
  mqtt.loop();

  if (rfid.PICC_IsNewCardPresent() &&  rfid.PICC_ReadCardSerial())
  {
    String id = lerRFID();
    Serial.println("UID: " + id);

    mqtt.publish("totem/id", id);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }

  servidor.handleClient();
}
