#include <WiFi.h>
#include <WiFiClient.h>
#include <MQTT.h>
#include <SPI.h>
#include <MFRC522.h>

#include "credenciais/credenciais.h"


MFRC522 rfid(46, 17);
MFRC522::MIFARE_Key chaveA = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};


WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000);
WebServer servidor(80);



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



void setup()
{
    Serial.begin(115200);
    iniciarRFID();
    
    SPI.begin();
    rfid.PCD_Init();
    
    conexaoSegura.setCACert(certificado1);
    mqtt.begin("mqtt.janks.dev.br", 1883, conexaoSegura);
    mqtt.setKeepAlive(10);
    
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
        
        mqtt.publish("totem/rfid", id);
        
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
    }
}
