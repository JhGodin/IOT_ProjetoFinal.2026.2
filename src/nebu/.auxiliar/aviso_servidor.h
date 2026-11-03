#pragma once

// ---------------------------------------------------------------------------
// Liga no MQTT e chama avisoMostra()/avisoLimpa() quando o servidor publica
// em "totem/aviso". WiFi e credenciais do broker ainda são PLACEHOLDER.
// ---------------------------------------------------------------------------

void avisoServidorBegin();

/// Chamar a cada volta do loop(), para manter a conexão e processar mensagens.
void avisoServidorLoop();
