#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Totem de SAÍDA: lê a pulseira, mostra o QR de pagamento, despede.
//
//   AguardandoRFID --(chip lido)--------> MostrandoPix
//   MostrandoPix   --(pagamento ok)-----> Despedida
//   MostrandoPix   --(60 s sem pagar)---> AguardandoRFID
//   Despedida      --(botão | 15 s)-----> AguardandoRFID
// ---------------------------------------------------------------------------

enum class SaidaEstado : uint8_t {
  AguardandoRFID,
  MostrandoPix,
  Despedida,
};

void totemSaidaBegin();
void totemSaidaLoop();

SaidaEstado totemSaidaEstado();
