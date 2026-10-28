#include "teste_telas.h"

#include "display.h"
#include "equipe.h"
#include "telas.h"

static const uint32_t PASSO_MS = 6000;

// -1 = passa por todas em sequência.
// 0..testeTelasQuantas()-1 = trava nessa tela, para ajustar layout com calma.
static const int TESTE_FIXA = 10;

static uint8_t  _atual = 0;
static uint32_t _desde = 0;
static bool     _sujo  = true;

// URL curta, como pixCobranca() deve devolver de verdade.
static const char PIX_CURTO[] = "https://fest.xy/p/AB12";

// Payload PIX completo, só para ver o caminho de erro: não cabe em 128 px e
// telaSaidaPix() tem que mostrar "Erro no pagamento" em vez de um QR ilegível.
static const char PIX_LONGO[] =
    "00020126580014BR.GOV.BCB.PIX0136123e4567-e12b-3456-a789-0123456789ab"
    "5204000053039865802BR5925FESTIVAL EXEMPLO LTDA ME6009SAO PAULO"
    "62070503***63041D3D";

static void desenha(uint8_t i) {
  switch (i) {
    case 0:
      Serial.println("[0] telaEntradaIngresso");
      telaEntradaIngresso("https://exemplo.com/cadastro");
      break;
    case 1:
      Serial.println("[1] telaEntradaPegaRFID");
      telaEntradaPegaRFID();
      break;
    case 2:
      Serial.println("[2] telaSaidaPassaRFID");
      telaSaidaPassaRFID();
      break;
    case 3:
      Serial.println("[3] telaSaidaPix (URL curta - deve escanear)");
      telaSaidaPix(PIX_CURTO);
      break;
    case 4:
      Serial.println("[4] telaSaidaPix (payload longo - deve dar erro)");
      telaSaidaPix(PIX_LONGO);
      break;
    case 5:
      Serial.println("[5] telaSaidaAdeus (nome curto)");
      telaSaidaAdeus("Ana");
      break;
    case 6:
      Serial.println("[6] telaSaidaAdeus (nome longo - testa estouro)");
      telaSaidaAdeus("Maria Aparecida");
      break;
    case 7:
      Serial.println("[7] telaComidaPassaRFID");
      telaComidaPassaRFID();
      break;
    case 8:
      Serial.println("[8] telaComidaCardapio (link com o ID do usuario)");
      telaComidaCardapio("http://192.168.4.1/cardapio/04A1B2C3", "Ana");
      break;
    case 9: {
      Serial.println("[9] telaPalco");
      Show show;
      show.artista = "Baiana System";
      show.palco   = "Palco Principal";
      show.horario = "22:30";
      telaPalco(show);
      break;
    }
    case 10:
      Serial.println("[10] telaAviso (invertida)");
      telaAviso("EVACUAR", "Siga a equipe");
      break;
    default:
      break;
  }
}

uint8_t testeTelasQuantas() { return 11; }

void testeTelasBegin() {
  Serial.println("=== teste de telas ===");
  Serial.printf("painel: %d x %d\n", telaLargura(), telaAltura());

  _atual = (TESTE_FIXA >= 0) ? (uint8_t)TESTE_FIXA : 0;
  _desde = millis();
  _sujo  = true;
}

void testeTelasLoop() {
  if (_sujo) {
    desenha(_atual);
    _sujo  = false;
    _desde = millis();      // conta o tempo depois do refresh, não antes
    return;
  }

  if (TESTE_FIXA >= 0) return;   // travado numa tela só

  if (millis() - _desde >= PASSO_MS) {
    _atual = (_atual + 1) % testeTelasQuantas();
    _sujo  = true;
  }
}
