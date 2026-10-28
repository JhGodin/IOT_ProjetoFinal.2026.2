#pragma once

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <QRCodeGFX.h>

// ---------------------------------------------------------------------------
// Camada de hardware do painel + primitivas de desenho.
// Não conhece nada do festival: quem monta as telas é telas.h.
// ---------------------------------------------------------------------------

// Painel 2.9" GDEH029T94, 128x296 nativo. Com setRotation(3) -> 296x128.
using PainelEPD = GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT>;

extern PainelEPD            tela;
extern U8G2_FOR_ADAFRUIT_GFX fontes;
extern QRCodeGFX            qrcode;

/// Liga o painel e fixa o estado de fonte que não muda mais.
/// Chamar uma vez, no setup().
void displayBegin();

int16_t telaLargura();
int16_t telaAltura();

// --- Primitivas -------------------------------------------------------------

/// Começa uma tela nova: janela cheia e fundo branco.
/// Obrigatório antes de desenhar — e-ink é persistente, sem isso a tela nova
/// aparece em cima da anterior.
void telaComeca();

/// Manda para o painel. parcial=true é rápido e sem piscar, mas deixa fantasma
/// acumulado; usar só quando o layout é o mesmo e só os dados mudaram.
void telaTermina(bool parcial = false);

void texto(const uint8_t *fonte, int16_t x, int16_t y, const String &txt);

/// Centraliza horizontalmente medindo o texto, em vez de chutar o x.
void textoCentrado(const uint8_t *fonte, int16_t y, const String &txt);

/// Centraliza entre x0 e x1, para telas divididas em QR + texto.
void textoCentradoEntre(const uint8_t *fonte, int16_t x0, int16_t x1, int16_t y,
                        const String &txt);

/// Desenha o maior QR que couber num quadrado de `lado` px, com o canto em
/// (x, y). false = nem na escala 1 cabe (conteúdo longo demais para o painel).
bool qrAjustado(const String &conteudo, int16_t x, int16_t y, int16_t lado);

/// Quantos px o QR de `conteudo` ocuparia na escala 1. Útil para decidir
/// layout antes de desenhar. 0 = não é representável.
int16_t qrLadoMinimo(const String &conteudo);
