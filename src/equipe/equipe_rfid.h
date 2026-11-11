#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// PLACEHOLDER — RFID é oficialmente módulo do colega (ver CLAUDE.md), mas
// nada foi entregue ainda aqui. Isto é só o mínimo pra equipe.cpp linkar e
// ser testável: "tem chip encostado? qual o UID?". Nenhuma validação,
// nenhuma leitura de dados gravados no chip (a pulseira não guarda nada —
// ver equipe.h). Apagar este arquivo e suas chamadas quando o módulo de
// verdade chegar.
// ---------------------------------------------------------------------------

void rfidBegin();

/// UID no mesmo formato que ESP_login.cpp já publica em totem/rfid
/// ("04 0A B2 1C"). false = nenhum chip presente agora.
bool rfidLerUID(String &uid);
