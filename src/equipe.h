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

/// Lê só o UID do chip encostado no leitor AGORA, sem consultar o servidor.
/// false = nenhum chip presente neste instante (caso normal, não é erro).
/// Não bloqueia. Usado pela entrada, que só precisa do UID para anunciar a
/// chegada — nome/dívida não importam ali.
bool lerTag(String &uid);

/// Lê o chip encostado no leitor AGORA e busca nome/dívida no servidor.
/// false = nenhum chip presente, ou servidor não respondeu a tempo (mesmo
/// tratamento: o totem só continua esperando).
/// Não bloqueia por mais que um timeout curto: os totens chamam isso a cada
/// volta do loop.
bool lerUsuario(User &user);

/// Anuncia a chegada de uma pulseira ao servidor (publica o UID), para a
/// entrada. Quem faz o cadastro de verdade é o site, no celular da pessoa —
/// o totem só avisa "essa pulseira acabou de passar aqui".
/// false = não deu para publicar agora (sem rede); o totem tenta de novo.
///
/// EQUIPE: isto substitui o fluxo antigo de ingresso em QR + gravação de
/// pulseira em branco (validarIngresso/gravarRFID). Esse modelo foi
/// abandonado: o cadastro (nome+CPF) já acontece pelo site, e a pulseira só
/// carrega um UID — nada é escrito nela pelo totem.
bool anunciarChegada(const String &uid);

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
