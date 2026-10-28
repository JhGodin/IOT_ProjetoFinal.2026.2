#include "display.h"

#include "pinos.h"

PainelEPD tela(GxEPD2_290_T94_V2(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX fontes;
QRCodeGFX qrcode(tela);

// Escala máxima por qrAjustado(), de cima para baixo.
static const uint16_t QR_ESCALA_MAX = 8;

void displayBegin() {
  tela.init();
  tela.setRotation(3);          // paisagem: 296 x 128

  fontes.begin(tela);
  fontes.setFontMode(1);
  fontes.setForegroundColor(GxEPD_BLACK);
  fontes.setBackgroundColor(GxEPD_WHITE);

  qrcode.setColors(GxEPD_WHITE, GxEPD_BLACK);

  telaComeca();
  telaTermina(false);           // primeiro refresh limpa fantasma de boot
}

int16_t telaLargura() { return tela.width(); }
int16_t telaAltura()  { return tela.height(); }

void telaComeca() {
  tela.setFullWindow();
  tela.fillScreen(GxEPD_WHITE);
}

void telaTermina(bool parcial) {
  tela.display(parcial);
}

void texto(const uint8_t *fonte, int16_t x, int16_t y, const String &txt) {
  fontes.setFont(fonte);
  fontes.setCursor(x, y);
  fontes.print(txt);
}

void textoCentrado(const uint8_t *fonte, int16_t y, const String &txt) {
  fontes.setFont(fonte);
  fontes.setFontMode(1);
  const int16_t largura = fontes.getUTF8Width(txt.c_str());
  fontes.setCursor((telaLargura() - largura) / 2, y);
  fontes.print(txt);
}

int16_t qrLadoMinimo(const String &conteudo) {
  if (!qrcode.generateData(conteudo)) return 0;
  qrcode.setScale(1);
  const int16_t lado = qrcode.getSideLength();
  qrcode.releaseData();
  return lado;
}

bool qrAjustado(const String &conteudo, int16_t x, int16_t y, int16_t lado) {
  if (!qrcode.generateData(conteudo)) {
    Serial.println("QR: conteúdo não representável");
    return false;
  }
  for (uint16_t escala = QR_ESCALA_MAX; escala >= 1; escala--) {
    qrcode.setScale(escala);
    if (qrcode.getSideLength() <= lado) {
      qrcode.setScale(escala);
      return qrcode.draw(x, y, true);
    }
  }
  // Nem na escala 1: o QR tem mais módulos que o espaço disponível.
  Serial.printf("QR: precisa de %d px, só tem %d\n", qrcode.getSideLength(), lado);
  qrcode.releaseData();
  return false;
}

void textoCentradoEntre(const uint8_t *fonte, int16_t x0, int16_t x1, int16_t y,
                        const String &txt) {
  fontes.setFont(fonte);
  const int16_t largura = fontes.getUTF8Width(txt.c_str());
  fontes.setCursor(x0 + ((x1 - x0) - largura) / 2, y);
  fontes.print(txt);
}
