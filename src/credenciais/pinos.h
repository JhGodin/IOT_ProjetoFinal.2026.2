#pragma once

// ---------------------------------------------------------------------------
// Único lugar onde pinos são definidos. Conferir com a equipe antes de mudar:
// o MFRC522 (RFID) divide o barramento SPI com o painel e-ink.
// Valores ainda são PLACEHOLDER até a fiação real estar definida.
// ---------------------------------------------------------------------------

// --- Painel e-ink 2.9" (SPI) ---
#define PIN_EPD_CS       10
#define PIN_EPD_DC       14
#define PIN_EPD_RST      15
#define PIN_EPD_BUSY     16
// SPI padrão do ESP32-S3 (FSPI): SCK=12, MOSI=11, MISO=13.
// O MFRC522 reaproveita SCK/MOSI/MISO e precisa de CS e RST próprios.

// --- Leitor de QR por serial (UART1) ---
#define PIN_LEITOR_RX    47
#define PIN_LEITOR_TX    48   // ATENÇÃO: GPIO48 é o LED RGB na DevKitC-1

// --- Botão "próximo / reiniciar" ---
#define PIN_BOTAO        1
