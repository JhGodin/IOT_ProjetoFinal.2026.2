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
  return true;
}
