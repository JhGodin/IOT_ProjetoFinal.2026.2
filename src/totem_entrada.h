#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Totem de ENTRADA: lê o QR do ingresso, grava a pulseira RFID da pessoa.
//
//   AguardandoIngresso --(QR válido)--> EntregandoRFID
//   EntregandoRFID  --(botão | 30 s)--> AguardandoIngresso
// ---------------------------------------------------------------------------

enum class EntradaEstado : uint8_t {
  AguardandoIngresso,   // QR de cadastro na tela, leitor ligado
  EntregandoRFID,       // "Pegue seu RFID", leitor desligado
};

void totemEntradaBegin();
void totemEntradaLoop();

EntradaEstado totemEntradaEstado();
