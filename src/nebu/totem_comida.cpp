#include "totem_comida.h"

#include "aviso.h"
#include "equipe.h"
#include "telas.h"

// Tempo que o QR fica na tela. Tem que dar para abrir a câmera e escanear sem
// pressa, e ser curto o bastante para a fila andar — o pedido continua vivo no
// celular depois que o totem volta para a tela inicial.
static const uint32_t TEMPO_QR_MS = 30000;

static ComidaEstado estado      = ComidaEstado::EsperandoRFID;
static uint32_t     estadoDesde = 0;
static String       linkQR;
static String       nome;

static void desenha() {
  switch (estado) {
    case ComidaEstado::EsperandoRFID: telaComidaPassaRFID();        break;
    case ComidaEstado::MostrandoQR:   telaComidaCardapio(linkQR, nome); break;
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
  entra(ComidaEstado::EsperandoRFID);
}

void totemComidaLoop() {
  if (avisoTrata()) return;
  if (avisoAcabou()) desenha();

  switch (estado) {
    case ComidaEstado::EsperandoRFID: {
      // Uma única ida-e-volta MQTT pra nome+link, em vez de lerUsuario() +
      // linkCardapio() encadeados (dois round-trips, dois pontos de falha
      // pra uma coisa só). Comida não precisa de dívida, então não precisa
      // de lerUsuario().
      String novoNome, novoLink;
      if (!buscarComida(novoNome, novoLink)) break;

      if (!novoLink.length()) {
        Serial.println("Sem link de cardapio");
        break;
      }

      linkQR = novoLink;
      nome = novoNome;
      entra(ComidaEstado::MostrandoQR);
      break;
    }

    case ComidaEstado::MostrandoQR: {
      if (passou(TEMPO_QR_MS)) {
        // Não interessa se a pessoa já terminou o pedido: daqui em diante é
        // ela e o servidor. O totem libera a vez.
        linkQR = String();
        nome = String();
        entra(ComidaEstado::EsperandoRFID);
      }
      break;
    }
  }
}

ComidaEstado totemComidaEstado() { return estado; }
