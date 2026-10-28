#include "totem_saida.h"

#include <GFButton.h>

#include "aviso.h"
#include "equipe.h"
#include "pinos.h"
#include "telas.h"

static const uint32_t TIMEOUT_PAGAMENTO_MS = 60000;
static const uint32_t TIMEOUT_DESPEDIDA_MS = 15000;

static SaidaEstado estado      = SaidaEstado::AguardandoRFID;
static uint32_t    estadoDesde = 0;
static GFButton &botao() {
  static GFButton b(PIN_BOTAO);
  return b;
}

// Pessoa sendo atendida agora. Só vale em MostrandoPix e Despedida.
static User   userAtual;
static String pixAtual;

static void desenha() {
  switch (estado) {
    case SaidaEstado::AguardandoRFID: telaSaidaPassaRFID();      break;
    case SaidaEstado::MostrandoPix:   telaSaidaPix(pixAtual);    break;
    case SaidaEstado::Despedida:      telaSaidaAdeus(userAtual.nome); break;
  }
}

static void entra(SaidaEstado novo) {
  estado      = novo;
  estadoDesde = millis();
  desenha();
}

static bool passou(uint32_t ms) {
  return millis() - estadoDesde >= ms;
}

void totemSaidaBegin() {
  botao();
  entra(SaidaEstado::AguardandoRFID);
}

void totemSaidaLoop() {
  botao().process();

  if (avisoTrata()) return;
  if (avisoAcabou()) desenha();

  switch (estado) {
    case SaidaEstado::AguardandoRFID: {
      User user;
      if (!lerRFID(user)) break;        // nenhum chip agora: volta no próximo loop

      userAtual = user;
      pixAtual  = pixCobranca(userAtual);
      Serial.println("Saída: " + userAtual.nome + " deve " + String(userAtual.divida, 2));
      entra(SaidaEstado::MostrandoPix);
      break;
    }

    case SaidaEstado::MostrandoPix: {
      if (pagamentoConfirmado(userAtual)) {
        entra(SaidaEstado::Despedida);
      } else if (passou(TIMEOUT_PAGAMENTO_MS)) {
        // DECISÃO DA EQUIPE: por ora só libera o totem e a dívida continua na
        // pulseira. Se a saída tiver catraca, isso aqui tem que avisar alguém.
        Serial.println("Pagamento não recebido: liberando o totem");
        entra(SaidaEstado::AguardandoRFID);
      }
      break;
    }

    case SaidaEstado::Despedida: {
      if (botao().wasPressed() || passou(TIMEOUT_DESPEDIDA_MS)) {
        entra(SaidaEstado::AguardandoRFID);
      }
      break;
    }
  }
}

SaidaEstado totemSaidaEstado() { return estado; }
