#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Totem de COMIDA/BEBIDA: mostra o QR do AP com o cardápio, espera o pedido
// chegar por POST e debita na pulseira.
//
//   MostrandoCardapio --(alguém abriu o cardápio)--> AguardandoPedido
//   AguardandoPedido  --(POST /pedido)-------------> CobrandoRFID
//   AguardandoPedido  --(90 s sem pedido)----------> MostrandoCardapio
//   CobrandoRFID      --(chip lido -> debita)-----> MostrandoCardapio
//   CobrandoRFID      --(60 s sem chip)-----------> MostrandoCardapio
// ---------------------------------------------------------------------------

enum class ComidaEstado : uint8_t {
  MostrandoCardapio,
  AguardandoPedido,
  CobrandoRFID,
};

void totemComidaBegin();
void totemComidaLoop();

ComidaEstado totemComidaEstado();
