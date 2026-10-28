#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Totem de COMIDA/BEBIDA: identifica a pessoa pelo RFID e devolve o QR do
// cardápio dela. O pedido em si é feito no celular, contra o AP/servidor —
// o totem sai do caminho assim que o QR foi mostrado.
//
//   EsperandoRFID --(chip lido -> servidor devolve o link)--> MostrandoQR
//   MostrandoQR   --(tempo de escanear acabou)-------------> EsperandoRFID
//
// Não há estado de cobrança: quem fecha o pedido (cozinha + débito) é o
// servidor, então duas pessoas podem estar pedindo no celular enquanto o
// totem já atende a terceira.
// ---------------------------------------------------------------------------

enum class ComidaEstado : uint8_t {
  EsperandoRFID,
  MostrandoQR,
};

void totemComidaBegin();
void totemComidaLoop();

ComidaEstado totemComidaEstado();
