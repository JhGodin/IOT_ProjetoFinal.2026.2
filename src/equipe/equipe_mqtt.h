#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// WiFi + MQTT, compartilhado pelos quatro papéis de totem. Não toca em
// telas/display — só dados. Ver o contrato de tópicos no plano de
// integração (equipe.h / README do time): servidor ainda não implementa o
// lado dele, então toda chamada aqui cai em timeout até isso existir — e
// timeout aqui é o mesmo "estado normal" que o stub antigo já tratava.
// ---------------------------------------------------------------------------

// Tópicos já em produção (ESP_login.cpp / sites.py).
#define TOPICO_RFID               "totem/rfid"

// Tópicos novos deste contrato — AINDA SEM SERVIDOR DO OUTRO LADO.
#define TOPICO_USUARIO_PEDIDO     "totem/usuario/pedido"
#define TOPICO_USUARIO_RESPOSTA   "totem/usuario/resposta"
#define TOPICO_PIX_PEDIDO         "totem/pix/pedido"
#define TOPICO_PIX_RESPOSTA       "totem/pix/resposta"
#define TOPICO_PAGAMENTO_RESPOSTA "totem/pagamento/resposta"
#define TOPICO_CARDAPIO_PEDIDO    "totem/cardapio/pedido"
#define TOPICO_CARDAPIO_RESPOSTA  "totem/cardapio/resposta"
#define TOPICO_PALCO_SHOWS        "totem/palco/shows"

void equipeMqttBegin();

/// Chamar a cada volta do loop do totem. Não bloqueia: só mantém a conexão
/// e processa mensagens pendentes.
void equipeMqttLoop();

bool equipeMqttPronto();

bool equipeMqttPublicar(const char *topico, const String &payload);

/// Publica {"uid":"<uid>"} no tópico de pedido indicado.
bool equipeMqttPedir(const char *topicoPedido, const String &uid);

/// Última resposta ainda não consumida para esse tópico. true só uma vez por
/// mensagem nova; chamadas seguintes devolvem false até a próxima chegar.
bool equipeMqttResposta(const char *topicoResposta, String &jsonRecebido);

/// Espera até `timeoutMs` por uma resposta em `topicoResposta` cujo campo
/// "uid" bata com `uid` (ignora respostas de outros totens no mesmo tópico
/// compartilhado). Bombeia a conexão MQTT internamente, então bloqueia até
/// o timeout — aceitável porque só é chamada quando um chip acabou de ser
/// lido, não a cada volta ociosa do loop.
bool equipeMqttAguardar(const char *topicoResposta, const String &uid,
                         uint32_t timeoutMs, String &jsonRecebido);

/// Extrai um campo de um JSON raso de um nível só ("chave":"valor" ou
/// "chave":numero/booleano). Não é um parser de verdade — serve só para os
/// payloads pequenos e fixos deste contrato. Campo ausente -> String vazia.
String jsonCampo(const String &json, const String &chave);
