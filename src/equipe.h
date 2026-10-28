#pragma once

#include <Arduino.h>

// ===========================================================================
//  CONTRATO COM O RESTO DA EQUIPE
// ---------------------------------------------------------------------------
//  Nada aqui é implementado pela parte de desenho/ordenação. Este arquivo é a
//  lista do que os colegas precisam entregar, escrita como código para que os
//  totens já compilem e possam ser revisados antes da integração.
// ===========================================================================

// --- Dados que o chip RFID carrega -----------------------------------------
// Uma leitura bem-sucedida devolve isto preenchido. É tudo o que a pulseira
// guarda, então o totem nunca precisa consultar o servidor para identificar
// alguém.
struct User {
  String uid;             // UID do chip
  String nome;
  float  divida = 0.0f;   // consumo acumulado, em BRL
};

// --- Pedido já precificado pelo servidor ------------------------------------
// EQUIPE: o handler POST de "/pedido" no WebServer precisa devolver um destes.
// O total tem que ser calculado NO SERVIDOR a partir dos itens; nunca aceitar
// um "total" vindo do POST, senão o cliente escolhe quanto quer pagar.
struct PendingOrder {
  bool  pronto = false;   // true = há um pedido novo esperando pagamento
  float total  = 0.0f;    // em BRL
};

// --- Show, empurrado pelo servidor IoT para os totens de palco -------------
struct Show {
  String artista;
  String palco;
  String horario;         // "22:30"
};

// --- RFID ----------------------------------------------

/// Lê o chip encostado no leitor AGORA.
/// false = nenhum chip presente neste instante (caso normal, não é erro).
/// Não bloqueia: os totens chamam isso a cada volta do loop.
bool lerRFID(User &user);

/// Grava um cadastro novo num chip em branco, na entrada.
bool gravarRFID(const User &user);

/// Desconta `valor` da pulseira e persiste o novo saldo.
bool debitarUser(const User &user, float valor);

// --- Ingressos / cadastro ------------------------------

/// Valida o código lido do ingresso e devolve o usuário correspondente.
/// false = ingresso inválido, já usado, ou servidor fora do ar.
bool validarIngresso(const String &codigo, User &user);

// --- Pagamento na saída --------------------------------

/// Devolve o que vai dentro do QR de pagamento.
String pixCobranca(const User &user);

/// true quando o pagamento daquele usuário caiu.
bool pagamentoConfirmado(const User &user);

// --- Cardápio / AP WiFi ------------------------------

/// true enquanto alguém está conectado ao AP com o cardápio aberto.
/// Usado para esconder o QR e atender uma pessoa por vez.
bool clienteNoCardapio();

/// Pega o próximo pedido já precificado, se houver.
/// false = nada novo desde a última chamada.
bool pedidoPendente(PendingOrder &pedido);
