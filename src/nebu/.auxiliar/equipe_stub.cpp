// ===========================================================================
//  APAGAR ESTE ARQUIVO NA INTEGRAÇÃO
// ---------------------------------------------------------------------------
//  Implementações vazias do contrato em equipe.h, só para o projeto linkar
//  enquanto as partes dos colegas não existem. Nenhuma delas faz nada útil:
//  as que devolvem bool devolvem false, então os totens ficam parados no
//  estado inicial — que é o comportamento correto para "ninguém chegou ainda".
// ===========================================================================

#include "equipe.h"

bool lerRFID(User &user) {
  (void)user;
  return false;             // nenhum chip no leitor
}

bool gravarRFID(const User &user) {
  (void)user;
  return false;
}

bool debitarUser(const User &user, float valor) {
  (void)user;
  (void)valor;
  return false;
}

bool validarIngresso(const String &codigo, User &user) {
  (void)codigo;
  (void)user;
  return false;
}

String pixCobranca(const User &user) {
  (void)user;
  return String();
}

bool pagamentoConfirmado(const User &user) {
  (void)user;
  return false;
}

String linkCardapio(const User &user) {
  (void)user;
  return String();          // sem AP: nenhum cardápio para mostrar
}
