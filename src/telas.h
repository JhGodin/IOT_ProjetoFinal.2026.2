#pragma once

#include <Arduino.h>

#include "equipe.h"

// ---------------------------------------------------------------------------
// As telas do festival. Cada função desenha UMA tela completa: limpa o painel,
// desenha e faz o refresh. Elas não decidem nada e não leem sensor nenhum —
// recebem dados prontos. Quem decide o que aparece é a máquina de estados do
// totem correspondente.
//
// Só as máquinas de estado chamam estas funções, e só na transição: cada
// refresh do e-ink bloqueia ~1-2 s e gasta vida útil do painel.
// ---------------------------------------------------------------------------

// --- Totem de entrada ---
void telaEntradaIngresso(const String &linkCadastro);
void telaEntradaPegaRFID();

// --- Totem de saída ---
void telaSaidaPassaRFID();
void telaSaidaPix(const String &conteudoQR);
void telaSaidaAdeus(const String &nome);

// --- Totem de comida ---
void telaComidaCardapio(const String &linkAP);
void telaComidaAguardaPedido();
void telaComidaPassaRFID(float total);

// --- Totem de palco (sem estado: o servidor empurra o próximo show) ---
void telaPalco(const Show &show);

// --- Sobrepõe qualquer totem (evacuação, recado do organizador) ---
void telaAviso(const String &titulo, const String &corpo);
