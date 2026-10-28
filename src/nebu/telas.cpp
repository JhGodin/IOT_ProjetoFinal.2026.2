#include "telas.h"

#include "display.h"

// Fontes usadas (todas existem no U8g2_for_Adafruit_GFX; o sufixo _te cobre
// os acentos — trocar para _tr comeria "ã", "á", "é" silenciosamente).
#define FONTE_GRANDE  u8g2_font_helvB18_te
#define FONTE_MEDIA   u8g2_font_helvB14_te
#define FONTE_PEQUENA u8g2_font_helvB10_te
#define FONTE_SETA    u8g2_font_open_iconic_arrow_2x_t

// Layout do painel em paisagem: 296 x 128.
// Telas com QR usam a metade esquerda para o código e a direita para o texto.
static const int16_t QR_X    = 8;
static const int16_t QR_Y    = 34;
static const int16_t QR_LADO = 86;      // cabe em 128 - 34 - folga
static const int16_t COL_DIR = 108;     // onde começa a coluna de texto

// ---------------------------------------------------------------------------
// Entrada
// ---------------------------------------------------------------------------

void telaEntradaIngresso(const String &linkCadastro) {
  telaComeca();

  textoCentrado(FONTE_MEDIA, 24, "Escaneie seu ingresso!");

  qrAjustado(linkCadastro, QR_X, QR_Y, QR_LADO);

  textoCentradoEntre(FONTE_PEQUENA, COL_DIR, telaLargura(), 70,
                     "Ainda não tem ingresso?");
  textoCentradoEntre(FONTE_PEQUENA, COL_DIR, telaLargura(), 90,
                     "Cadastre-se na hora!");

  // Seta apontando do texto para o QR.
  texto(FONTE_SETA, QR_X + QR_LADO + 4, 84, "A");

  telaTermina(false);
}

void telaEntradaPegaRFID() {
  telaComeca();
  textoCentrado(FONTE_GRANDE, 74, "Pegue seu RFID");
  telaTermina(false);
}

// ---------------------------------------------------------------------------
// Saída
// ---------------------------------------------------------------------------

void telaSaidaPassaRFID() {
  telaComeca();
  textoCentrado(FONTE_GRANDE, 74, "Passe seu RFID");
  telaTermina(false);
}

void telaSaidaPix(const String &conteudoQR) {
  telaComeca();

  // Painel de 128 px de altura: o copia-e-cola PIX inteiro (150-250 chars)
  // NÃO cabe de forma escaneável. Por isso pixCobranca() tem que devolver uma
  // URL curta que redireciona para o pagamento. qrAjustado() devolve false e
  // loga o tamanho necessário se alguém mandar o payload inteiro.
  const int16_t lado = telaAltura() - 2 * 6;
  if (!qrAjustado(conteudoQR, 6, 6, lado)) {
    textoCentrado(FONTE_MEDIA, 60, "Erro no pagamento");
    textoCentrado(FONTE_PEQUENA, 84, "Procure a organização");
    telaTermina(false);
    return;
  }

  const int16_t colTexto = 6 + lado + 10;
  textoCentradoEntre(FONTE_MEDIA, colTexto, telaLargura(), 58, "Pague com PIX");
  textoCentradoEntre(FONTE_PEQUENA, colTexto, telaLargura(), 82, "Escaneie para");
  textoCentradoEntre(FONTE_PEQUENA, colTexto, telaLargura(), 100, "finalizar");

  telaTermina(false);
}

void telaSaidaAdeus(const String &nome) {
  telaComeca();
  // Duas linhas: nome comprido não estoura a largura.
  textoCentrado(FONTE_MEDIA, 58, "Até a próxima,");
  textoCentrado(FONTE_GRANDE, 92, nome + " :)");
  telaTermina(false);
}

// ---------------------------------------------------------------------------
// Comida
// ---------------------------------------------------------------------------

void telaComidaPassaRFID() {
  telaComeca();
  textoCentrado(FONTE_MEDIA, 50, "Passe seu RFID");
  textoCentrado(FONTE_MEDIA, 80, "pra ver o cardápio");
  telaTermina(false);
}

void telaComidaCardapio(const String &linkCardapio, const String &nome) {
  telaComeca();

  // O link já vem com o ID do usuário dentro, então este QR é só dele: o
  // pedido feito no celular sai no nome certo sem ninguém digitar login.
  if (!qrAjustado(linkCardapio, QR_X, QR_Y, QR_LADO)) {
    textoCentrado(FONTE_MEDIA, 60, "Erro no cardápio");
    textoCentrado(FONTE_PEQUENA, 84, "Procure a organização");
    telaTermina(false);
    return;
  }

  textoCentrado(FONTE_MEDIA, 24, nome.length() ? ("Oi, " + nome + "!") : "Nosso cardápio");

  textoCentradoEntre(FONTE_PEQUENA, COL_DIR, telaLargura(), 70,
                     "Escaneie e peça");
  textoCentradoEntre(FONTE_PEQUENA, COL_DIR, telaLargura(), 90,
                     "pelo celular");

  telaTermina(false);
}

// ---------------------------------------------------------------------------
// Palco
// ---------------------------------------------------------------------------

void telaPalco(const Show &show) {
  telaComeca();

  textoCentrado(FONTE_PEQUENA, 22, show.palco);
  tela.drawFastHLine(20, 30, telaLargura() - 40, GxEPD_BLACK);

  textoCentrado(FONTE_GRANDE, 74, show.artista);
  textoCentrado(FONTE_MEDIA, 110, show.horario);

  telaTermina(false);
}

// ---------------------------------------------------------------------------
// Aviso
// ---------------------------------------------------------------------------

void telaAviso(const String &titulo, const String &corpo) {
  // Invertido (fundo preto) para destacar de qualquer tela normal do totem.
  tela.setFullWindow();
  tela.fillScreen(GxEPD_BLACK);
  fontes.setForegroundColor(GxEPD_WHITE);

  textoCentrado(FONTE_GRANDE, 56, titulo);
  textoCentrado(FONTE_MEDIA, 92, corpo);

  fontes.setForegroundColor(GxEPD_BLACK);   // restaura para as telas normais
  telaTermina(false);
}
