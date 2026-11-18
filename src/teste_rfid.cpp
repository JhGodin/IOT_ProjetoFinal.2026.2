// Teste isolado do leitor RFID: acende o LED RGB onboard (verde) por 1s
// quando le um cartao. Nao faz parte do firmware do totem -- ver
// env:teste_rfid no platformio.ini.
//   pio run -e teste_rfid -t upload && pio device monitor -e teste_rfid

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#include "credenciais/pinos.h"

static MFRC522 rfid(PIN_RFID_CS, PIN_RFID_RST);

void setup() {
  Serial.begin(115200);
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);   // apagado (onboard, GPIO48 na DevKitC-1)

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Teste RFID: aproxime um cartao ou pulseira");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  Serial.print("Leu UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  rgbLedWrite(RGB_BUILTIN, 0, 64, 0);  // verde por 1s
  delay(1000);
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
