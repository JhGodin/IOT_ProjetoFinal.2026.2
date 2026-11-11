#include "totem_entrada.h"

#include <GFButton.h>

#include "aviso.h"
#include "equipe.h"
#include "../credenciais/pinos.h"
#include "telas.h"

// --- Configuração -----------------------------------------------------------

/// PLACEHOLDER: página de cadastro do site, mostrada enquanto espera alguém.
static const char LINK_CADASTRO[] = "https://exemplo.com/cadastro";

static const uint32_t TIMEOUT_CONFIRMACAO_MS = 30000;

// --- Estado -----------------------------------------------------------------

static EntradaEstado estado      = EntradaEstado::AguardandoChegada;
static uint32_t      estadoDesde = 0;
// Construído na 1a chamada (já dentro do setup()), não na inicialização
// estática: o construtor do GFButton chama pinMode(), que antes do boot do
// core Arduino é comportamento indefinido.
static GFButton &botao() {
  static GFButton b(PIN_BOTAO);
  return b;
}

/// Desenha a tela do estado atual. Único lugar que chama tela*() neste módulo.
static void desenha() {
  switch (estado) {
    case EntradaEstado::AguardandoChegada:
      telaEntradaIngresso(LINK_CADASTRO);
      break;
    case EntradaEstado::AguardandoConfirmacao:
      telaEntradaAguardando();
      break;
  }
}

/// Única porta de entrada para trocar de estado: marca a hora e redesenha.
/// Com isso é impossível desenhar fora de uma transição.
static void entra(EntradaEstado novo) {
  estado      = novo;
  estadoDesde = millis();
  desenha();
}

static bool passou(uint32_t ms) {
  return millis() - estadoDesde >= ms;
}

// --- API --------------------------------------------------------------------

void totemEntradaBegin() {
  botao();   // configura o pino agora, dentro do setup()
  entra(EntradaEstado::AguardandoChegada);
}

void totemEntradaLoop() {
  botao().process();

  if (avisoTrata()) return;
  if (avisoAcabou()) desenha();

  switch (estado) {
    case EntradaEstado::AguardandoChegada: {
      String uid;
      if (!lerTag(uid)) break;   // nenhum chip agora: volta no próximo loop

      Serial.println("Chegada: " + uid);
      if (anunciarChegada(uid)) {
        entra(EntradaEstado::AguardandoConfirmacao);
      } else {
        Serial.println("Sem rede: não deu para anunciar a chegada");
      }
      break;
    }

    case EntradaEstado::AguardandoConfirmacao: {
      if (botao().wasPressed() || passou(TIMEOUT_CONFIRMACAO_MS)) {
        entra(EntradaEstado::AguardandoChegada);
      }
      break;
    }
  }
}

EntradaEstado totemEntradaEstado() { return estado; }
