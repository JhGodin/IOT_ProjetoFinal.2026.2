#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Modo de teste de layout — não é um totem.
//
// Passa por todas as telas com dados falsos, para conferir no painel real se
// texto cabe, se o QR escaneia e se nada se sobrepõe. Não usa RFID, leitor de
// QR nem botão: só precisa do ESP32-S3 e do e-paper ligados.
//
// Compilar com -DTOTEM_PAPEL=5 (TOTEM_TESTE). Ver TESTE_FIXA em teste_telas.cpp
// para travar numa tela só enquanto ajusta coordenadas.
// ---------------------------------------------------------------------------

void testeTelasBegin();
void testeTelasLoop();

/// Quantas telas o teste conhece.
uint8_t testeTelasQuantas();
