#include "totem_comida.h"

#include "aviso.h"
#include "equipe.h"
#include "telas.h"

/// PLACEHOLDER: o QR leva para o AP local; quem sobe o AP é o colega do
/// WebServer, então esta string sai de lá na integração.
static const char LINK_AP[] = "https://exemplo.com/cardapio";

static const uint32_t TIMEOUT_PEDIDO_MS = 90000;
static const uint32_t TIMEOUT_COBRANCA_MS = 60000;

static ComidaEstado estado      = ComidaEstado::MostrandoCardapio;
static uint32_t     estadoDesde = 0;
static PendingOrder pedido;

static void desenha() {
  switch (estado) {
    case ComidaEstado::MostrandoCardapio: telaComidaCardapio(LINK_AP);      break;
    case ComidaEstado::AguardandoPedido:  telaComidaAguardaPedido();        break;
    case ComidaEstado::CobrandoRFID:      telaComidaPassaRFID(pedido.total); break;
  }
}

static void entra(ComidaEstado novo) {
  estado      = novo;
  estadoDesde = millis();
  desenha();
}

static bool passou(uint32_t ms) {
  return millis() - estadoDesde >= ms;
}

void totemComidaBegin() {
  entra(ComidaEstado::MostrandoCardapio);
}

void totemComidaLoop() {
  if (avisoTrata()) return;
  if (avisoAcabou()) desenha();

  switch (estado) {
    case ComidaEstado::MostrandoCardapio: {
      // Esconder o QR enquanto alguém pede é o truque barato para atender uma
      // pessoa por vez. Não escala: duas pessoas já conectadas ao AP ainda
      // conseguem postar junto. A alternativa de verdade é amarrar a sessão
      // HTTP ao RFID, o que derrota o propósito do RFID (não digitar login).
      // DECISÃO DA EQUIPE: aceitar a corrida ou ter um totem por fila.
      if (clienteNoCardapio()) entra(ComidaEstado::AguardandoPedido);
      break;
    }

    case ComidaEstado::AguardandoPedido: {
      PendingOrder novo;
      if (pedidoPendente(novo) && novo.pronto) {
        pedido = novo;
        entra(ComidaEstado::CobrandoRFID);
      } else if (passou(TIMEOUT_PEDIDO_MS)) {
        entra(ComidaEstado::MostrandoCardapio);   // desistiu
      }
      break;
    }

    case ComidaEstado::CobrandoRFID: {
      User user;
      if (lerRFID(user)) {
        if (debitarUser(user, pedido.total)) {
          Serial.println("Debitado " + String(pedido.total, 2) + " de " + user.nome);
        } else {
          // EQUIPE: aqui falta o caminho de saldo insuficiente / erro de
          // gravação. Hoje o pedido é descartado do mesmo jeito.
          Serial.println("Falha ao debitar");
        }
        pedido = PendingOrder();
        entra(ComidaEstado::MostrandoCardapio);
      } else if (passou(TIMEOUT_COBRANCA_MS)) {
        // Pedido feito e não pago: a cozinha não pode produzir isso.
        // EQUIPE: precisa de um "cancelar pedido" no servidor.
        Serial.println("Pedido abandonado sem pagamento");
        pedido = PendingOrder();
        entra(ComidaEstado::MostrandoCardapio);
      }
      break;
    }
  }
}

ComidaEstado totemComidaEstado() { return estado; }
