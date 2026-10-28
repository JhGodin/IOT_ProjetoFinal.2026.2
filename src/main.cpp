// ===========================================================================
//  ANDAIME DE COMPILAÇÃO — NÃO É O main.cpp FINAL
// ---------------------------------------------------------------------------
//  O setup()/loop() de verdade sai na integração, quando as partes de todos
//  estiverem juntas. Isto existe só para:
//    1. o projeto linkar (o core Arduino exige setup() e loop());
//    2. mostrar como a API dos totens é usada.
//
//  Um totem é de função única: escolha o papel em TOTEM_PAPEL. Para trocar,
//  mude a linha do #define (ou passe -DTOTEM_PAPEL=... em build_flags).
// ===========================================================================

#include <Arduino.h>

#include "aviso.h"
#include "display.h"
#include "totem_comida.h"
#include "totem_entrada.h"
#include "totem_palco.h"
#include "totem_saida.h"

#define TOTEM_ENTRADA 1
#define TOTEM_SAIDA   2
#define TOTEM_COMIDA  3
#define TOTEM_PALCO   4

#ifndef TOTEM_PAPEL
#define TOTEM_PAPEL TOTEM_ENTRADA
#endif

void setup() {
  Serial.begin(115200);
  displayBegin();

#if TOTEM_PAPEL == TOTEM_ENTRADA
  totemEntradaBegin();
#elif TOTEM_PAPEL == TOTEM_SAIDA
  totemSaidaBegin();
#elif TOTEM_PAPEL == TOTEM_COMIDA
  totemComidaBegin();
#elif TOTEM_PAPEL == TOTEM_PALCO
  totemPalcoBegin();
#else
#error "TOTEM_PAPEL inválido"
#endif
}

void loop() {
#if TOTEM_PAPEL == TOTEM_ENTRADA
  totemEntradaLoop();
#elif TOTEM_PAPEL == TOTEM_SAIDA
  totemSaidaLoop();
#elif TOTEM_PAPEL == TOTEM_COMIDA
  totemComidaLoop();
#elif TOTEM_PAPEL == TOTEM_PALCO
  totemPalcoLoop();
#endif
}
