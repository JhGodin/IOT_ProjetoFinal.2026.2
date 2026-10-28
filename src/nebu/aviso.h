#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Aviso do organizador (recado, evacuação). Sobrepõe a tela de qualquer totem,
// seja ele de entrada, saída, comida ou palco — é a única coisa que fura o
// "um totem, uma função".
//
// Cada loop de totem começa com:
//     if (avisoTrata()) return;        // aviso na tela, FSM pausada
//     if (avisoAcabou()) redesenha();  // aviso saiu, retoma a tela própria
// ---------------------------------------------------------------------------

/// EQUIPE: chamar quando o servidor empurrar um aviso.
void avisoMostra(const String &titulo, const String &corpo);

/// EQUIPE: chamar quando o aviso for cancelado.
void avisoLimpa();

/// Desenha o aviso se ele acabou de chegar. true = o aviso está no painel.
bool avisoTrata();

/// true uma única vez, na volta do loop em que o aviso saiu da tela.
/// O totem usa isso para redesenhar o estado em que estava.
bool avisoAcabou();
