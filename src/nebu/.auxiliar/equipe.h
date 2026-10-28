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
bool lerUsuario(User &user);

/// Grava um cadastro novo num chip em branco, na entrada.
bool gravarRFID(const User &user);

/// Desconta `valor` da pulseira e persiste o novo saldo.
/// EQUIPE: nenhum totem chama mais isto — o pedido de comida é fechado pelo
/// servidor. Fica aqui só se a equipe quiser manter o saldo no próprio chip;
/// se o saldo passar a viver só no banco, esta função sai do contrato.
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

/// Devolve a URL do cardápio daquele usuário, já com o ID dele dentro
/// (algo como "http://192.168.4.1/cardapio/<uid>"), para virar QR.
/// String vazia = AP fora do ar ou usuário desconhecido; o totem volta para
/// a tela inicial em vez de desenhar um QR quebrado.
///
/// EQUIPE: o pedido inteiro acontece no celular, contra o AP/servidor — o
/// totem não participa dele. Então:
///   - o fechamento do pedido (cozinha + débito na conta) é server/banco,
///     e o totem já está livre para a próxima pessoa nesse momento.
String linkCardapio(const User &user);
