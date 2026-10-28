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
      User user;
      if (!lerUsuario(user)) break;

      // O link carrega o ID do usuário, então o pedido feito no celular já sai
      // no nome dele. Sem link não há QR escaneável: fica na tela inicial em
      // vez de gastar um refresh desenhando lixo.
      const String novo = linkCardapio(user);
      if (!novo.length()) {
        Serial.println("Sem link de cardapio para " + user.uid);
        break;
      }

      linkQR = novo;
      nome = user.nome;
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
