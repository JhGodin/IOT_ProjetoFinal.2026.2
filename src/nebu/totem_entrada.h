#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Totem de ENTRADA: lê a pulseira RFID de quem chega e avisa o servidor.
// O cadastro (nome+CPF) é feito pela própria pessoa no site, no celular —
// o totem só anuncia "essa pulseira passou aqui agora" (ver equipe.h).
//
//   AguardandoChegada --(UID publicado)--> AguardandoConfirmacao
//   AguardandoConfirmacao --(botão | 30 s)--> AguardandoChegada
// ---------------------------------------------------------------------------

enum class EntradaEstado : uint8_t {
  AguardandoChegada,       // QR de cadastro na tela, leitor RFID ligado
  AguardandoConfirmacao,   // "aguarde confirmação no celular"
};

void totemEntradaBegin();
void totemEntradaLoop();

EntradaEstado totemEntradaEstado();
