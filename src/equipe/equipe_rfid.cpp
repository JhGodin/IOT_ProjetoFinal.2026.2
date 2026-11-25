#include "equipe_rfid.h"

#include <SPI.h>
#include <MFRC522.h>

#include "../credenciais/pinos.h"

static MFRC522 rfid(PIN_RFID_CS, PIN_RFID_RST);

void rfidBegin() {
  SPI.begin();
  rfid.PCD_Init();
}

bool rfidLerUID(String &uid) {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return false;

  uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) uid += " ";
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  // Mesmo flash do teste_rfid.cpp: confirma visualmente, no firmware de
  // verdade, que o MFRC522 leu algo nesta leitura -- sem precisar trocar
  // pro env de teste isolado pra saber se o problema e a leitura em si.
  rgbLedWrite(RGB_BUILTIN, 0, 64, 0);
  delay(1000);
  rgbLedWrite(RGB_BUILTIN, 0, 0, 0);

  return true;
}
