#pragma once

#include <Arduino.h>

#include "equipe.h"

// ---------------------------------------------------------------------------
// Totem de PALCO: sem máquina de estados. Um servidor IoT empurra a lista dos
// próximos shows e o totem rotaciona entre eles.
//
// Só redesenha quando o show visível muda (ou quando a lista é atualizada):
// a cada ~1 min dá ~1400 refreshes/dia, o que é aceitável para um painel
// e-ink, mas redesenhar a cada volta do loop queimaria o painel em horas.
// ---------------------------------------------------------------------------

#define PALCO_MAX_SHOWS 8

void totemPalcoBegin();
void totemPalcoLoop();

/// EQUIPE: chamar quando o servidor empurrar a programação nova.
/// Copia a lista (não guarda o ponteiro), no máximo PALCO_MAX_SHOWS.
void totemPalcoSetShows(const Show *shows, uint8_t quantos);

/// Quantos shows estão na lista agora.
uint8_t totemPalcoQuantos();
