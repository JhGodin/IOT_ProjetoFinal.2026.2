#include "totem_entrada.h"

#include <GFButton.h>

#include "aviso.h"
#include "equipe.h"
#include "../credenciais/pinos.h"
#include "telas.h"

// --- Configuração -----------------------------------------------------------

/// PLACEHOLDER: página de cadastro para quem chega sem ingresso.
static const char LINK_CADASTRO[] = "https://exemplo.com/cadastro";

static const uint32_t TIMEOUT_ENTREGA_MS = 30000;
static const uint8_t  TAM_MIN_CODIGO     = 6;     // descarta ruído na serial

// Comandos do leitor de QR por serial. Terminam em '.', que é o próprio
// terminador do protocolo — por isso print() e não println(): o "\r\n" do
// println ia junto e alguns módulos recusam o comando. CONFERIR no datasheet.
static const char CMD_INIT[]    = "~M00910001.";
static const char CMD_LIGA[]    = "~M00210001.";
static const char CMD_CONFIG[]  = "~M00B00014.";
static const char CMD_DESLIGA[] = "~M00210003.";

// --- Estado -----------------------------------------------------------------

static EntradaEstado estado      = EntradaEstado::AguardandoIngresso;
static uint32_t      estadoDesde = 0;
// Construído na 1a chamada (já dentro do setup()), não na inicialização
// estática: o construtor do GFButton chama pinMode(), que antes do boot do
// core Arduino é comportamento indefinido.
static GFButton &botao() {
  static GFButton b(PIN_BOTAO);
  return b;
}

static void leitorComando(const char *cmd) {
  Serial1.print(cmd);
  delay(100);   // o módulo precisa de uma pausa entre comandos
}

/// Desenha a tela do estado atual. Único lugar que chama tela*() neste módulo.
static void desenha() {
  switch (estado) {
    case EntradaEstado::AguardandoIngresso:
      telaEntradaIngresso(LINK_CADASTRO);
      break;
    case EntradaEstado::EntregandoRFID:
      telaEntradaPegaRFID();
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

/// Código lido agora, ou string vazia. Não bloqueia.
static String leitorLeCodigo() {
  if (Serial1.available() <= 0) return String();
  String texto = Serial1.readStringUntil('\n');
  texto.trim();
  if (texto.length() < TAM_MIN_CODIGO) return String();
  return texto;
}

// --- API --------------------------------------------------------------------

void totemEntradaBegin() {
  botao();                     // configura o pino agora, dentro do setup()
  Serial1.begin(9600, SERIAL_8N1, PIN_LEITOR_RX, PIN_LEITOR_TX);
  leitorComando(CMD_INIT);
  leitorComando(CMD_LIGA);
  leitorComando(CMD_CONFIG);

  entra(EntradaEstado::AguardandoIngresso);
}

void totemEntradaLoop() {
  botao().process();

  if (avisoTrata()) return;
  if (avisoAcabou()) desenha();

  switch (estado) {
    case EntradaEstado::AguardandoIngresso: {
      const String codigo = leitorLeCodigo();
      if (codigo.length() == 0) break;

      Serial.println("Ingresso lido: " + codigo);
      leitorComando(CMD_DESLIGA);

      User user;
      if (validarIngresso(codigo, user) && gravarRFID(user)) {
        entra(EntradaEstado::EntregandoRFID);
      } else {
        Serial.println("Ingresso inválido ou falha ao gravar o chip");
        leitorComando(CMD_LIGA);        // libera para a próxima tentativa
      }
      break;
    }

    case EntradaEstado::EntregandoRFID: {
      if (botao().wasPressed() || passou(TIMEOUT_ENTREGA_MS)) {
        leitorComando(CMD_LIGA);
        entra(EntradaEstado::AguardandoIngresso);
      }
      break;
    }
  }
}

EntradaEstado totemEntradaEstado() { return estado; }
