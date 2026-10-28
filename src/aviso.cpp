#include "aviso.h"

#include "telas.h"

static String  _titulo;
static String  _corpo;
static bool    _ativo   = false;
static bool    _sujo    = false;   // precisa (re)desenhar o aviso
static bool    _acabou  = false;   // one-shot para o totem redesenhar

void avisoMostra(const String &titulo, const String &corpo) {
  // Texto novo com o aviso já na tela também precisa redesenhar.
  if (_ativo && titulo == _titulo && corpo == _corpo) return;
  _titulo = titulo;
  _corpo  = corpo;
  _ativo  = true;
  _sujo   = true;
}

void avisoLimpa() {
  if (!_ativo) return;
  _ativo  = false;
  _sujo   = false;
  _acabou = true;
}

bool avisoTrata() {
  if (!_ativo) return false;
  if (_sujo) {
    telaAviso(_titulo, _corpo);
    _sujo = false;
  }
  return true;
}

bool avisoAcabou() {
  const bool r = _acabou;
  _acabou = false;
  return r;
}
