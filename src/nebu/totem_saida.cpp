#include "totem_saida.h"

#include <GFButton.h>
#include <stdlib.h>

#include "aviso.h"
#include "equipe.h"
#include "equipe/equipe_mqtt.h"
#include "../credenciais/pinos.h"
#include "telas.h"

static const uint32_t TIMEOUT_PAGAMENTO_MS = 60000;
static const uint32_t TIMEOUT_DESPEDIDA_MS = 15000;
static const uint32_t TIMEOUT_QR_CODE_MS = 5000;
static const int ID_TOTEM = 1;

static SaidaEstado estado = SaidaEstado::AguardandoRFID;
static uint32_t estadoDesde = 0;
static GFButton &botao()
{
  static GFButton b(PIN_BOTAO);
  return b;
}

// Pessoa sendo atendida agora. Só vale em MostrandoPix e Despedida.
static User userAtual;
static String pixAtual;
static String pagamentoAtual;

static void desenha()
{
  switch (estado)
  {
  case SaidaEstado::AguardandoRFID:
    telaSaidaPassaRFID();
    break;
  case SaidaEstado::MostrandoPix:
    telaSaidaPix(pixAtual);
    break;
  case SaidaEstado::Despedida:
    telaSaidaAdeus(userAtual.name);
    break;
  }
}

static void entra(SaidaEstado novo)
{
  estado = novo;
  estadoDesde = millis();
  desenha();
}

static bool passou(uint32_t ms)
{
  return millis() - estadoDesde >= ms;
}

void totemSaidaBegin()
{
  botao();
  entra(SaidaEstado::AguardandoRFID);
}

void totemSaidaLoop()
{
  botao().process();

  if (avisoTrata())
    return;
  if (avisoAcabou())
    desenha();

  switch (estado)
  {
  case SaidaEstado::AguardandoRFID:
  {
    User user;
    if (!lerUsuario(user))
      break; // nenhum chip agora: volta no próximo loop

    userAtual = user;
    const uint64_t cpfCnpj = strtoull(userAtual.cpfCnpj.c_str(), nullptr, 10); // TODO: enviar como string
    const String valor(userAtual.divida, 2);

    if (!equipeMqttEnviarPedidoPix(valor, cpfCnpj, ID_TOTEM))
      break;
    pixAtual = "";
    pagamentoAtual = "";
    if (!equipeMqttAguardarQrCode(ID_TOTEM, TIMEOUT_QR_CODE_MS, pixAtual,
                                  pagamentoAtual))
      break;
    if (pixAtual.length() == 0 || pagamentoAtual.length() == 0)
      break; // servidor não respondeu: tenta de novo no próximo loop

    Serial.println("Saída: " + userAtual.name + " deve " + String(userAtual.divida, 2));
    entra(SaidaEstado::MostrandoPix);
    break;
  }

  case SaidaEstado::MostrandoPix:
  {
    if (pagamentoConfirmado(pagamentoAtual))
    {
      entra(SaidaEstado::Despedida);
    }
    else if (passou(TIMEOUT_PAGAMENTO_MS))
    {
      // DECISÃO DA EQUIPE: por ora só libera o totem e a dívida continua na
      // pulseira. Se a saída tiver catraca, isso aqui tem que avisar alguém.
      Serial.println("Pagamento não recebido: liberando o totem");
      entra(SaidaEstado::AguardandoRFID);
    }
    break;
  }

  case SaidaEstado::Despedida:
  {
    if (botao().wasPressed() || passou(TIMEOUT_DESPEDIDA_MS))
    {
      entra(SaidaEstado::AguardandoRFID);
    }
    break;
  }
  }
}

SaidaEstado totemSaidaEstado() { return estado; }
