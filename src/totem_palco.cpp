#include "totem_palco.h"

#include "aviso.h"
#include "telas.h"

static const uint32_t ROTACAO_MS = 60000;

static Show    _shows[PALCO_MAX_SHOWS];
static uint8_t _quantos = 0;
static uint8_t _atual   = 0;
static uint32_t _desde  = 0;
static bool    _sujo    = false;   // true = o painel não reflete _atual

void totemPalcoBegin() {
  _quantos = 0;
  _atual   = 0;
  _desde   = millis();
  _sujo    = false;
}

void totemPalcoSetShows(const Show *shows, uint8_t quantos) {
  if (shows == nullptr) quantos = 0;
  _quantos = quantos > PALCO_MAX_SHOWS ? PALCO_MAX_SHOWS : quantos;
  for (uint8_t i = 0; i < _quantos; i++) _shows[i] = shows[i];

  if (_atual >= _quantos) _atual = 0;
  _desde = millis();
  _sujo  = true;              // programação nova aparece na hora
}

void totemPalcoLoop() {
  if (avisoTrata()) return;
  if (avisoAcabou()) _sujo = true;

  if (_quantos == 0) return;  // nada empurrado ainda: mantém o painel como está

  if (_quantos > 1 && millis() - _desde >= ROTACAO_MS) {
    _atual = (_atual + 1) % _quantos;
    _desde = millis();
    _sujo  = true;
  }

  if (_sujo) {
    telaPalco(_shows[_atual]);
    _sujo = false;
  }
}

uint8_t totemPalcoQuantos() { return _quantos; }
