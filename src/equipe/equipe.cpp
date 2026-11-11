#include "../equipe.h"

#include "equipe_mqtt.h"
#include "equipe_rfid.h"

// Implementação real do contrato (ver equipe.h). Troca o antigo
// equipe_stub.cpp, que só devolvia false/"" pra tudo. Aqui a rede de
// verdade é usada; quando o servidor ainda não responde (nada do outro lado
// implementado para usuario/pix/pagamento/cardapio), o timeout cai e a
// função devolve o mesmo false/"" de antes — nenhum totem trata isso como
// erro, então o comportamento visível sem servidor nenhum é idêntico ao
// stub.

static const uint32_t TIMEOUT_USUARIO_MS  = 2000;
static const uint32_t TIMEOUT_PIX_MS      = 5000;
static const uint32_t TIMEOUT_CARDAPIO_MS = 3000;

bool lerTag(String &uid) {
  return rfidLerUID(uid);
}

bool anunciarChegada(const String &uid) {
  return equipeMqttPublicar(TOPICO_RFID, uid);
}

bool lerUsuario(User &user) {
  String uid;
  if (!rfidLerUID(uid)) return false;

  // Toda leitura (saída ou comida) também alimenta o fluxo de vínculo
  // UID<->CPF que o site/servidor já usa hoje (ver sites.py /rfid).
  equipeMqttPublicar(TOPICO_RFID, uid);

  if (!equipeMqttPedir(TOPICO_USUARIO_PEDIDO, uid)) return false;

  String json;
  if (!equipeMqttAguardar(TOPICO_USUARIO_RESPOSTA, uid, TIMEOUT_USUARIO_MS, json)) {
    return false;
  }
  if (jsonCampo(json, "erro").length() > 0) return false;

  user.uid    = uid;
  user.nome   = jsonCampo(json, "nome");
  user.divida = jsonCampo(json, "divida").toFloat();
  return true;
}

String pixCobranca(const User &user) {
  if (!equipeMqttPedir(TOPICO_PIX_PEDIDO, user.uid)) return "";

  String json;
  if (!equipeMqttAguardar(TOPICO_PIX_RESPOSTA, user.uid, TIMEOUT_PIX_MS, json)) {
    return "";
  }
  return jsonCampo(json, "payload");
}

bool pagamentoConfirmado(const User &user) {
  // Chamada a cada volta do loop em MostrandoPix (ver totem_saida.cpp) —
  // só confere o que já chegou, nunca espera.
  String json;
  if (!equipeMqttResposta(TOPICO_PAGAMENTO_RESPOSTA, json)) return false;
  if (jsonCampo(json, "uid") != user.uid) return false;
  return jsonCampo(json, "confirmado") == "true";
}

String linkCardapio(const User &user) {
  if (!equipeMqttPedir(TOPICO_CARDAPIO_PEDIDO, user.uid)) return "";

  String json;
  if (!equipeMqttAguardar(TOPICO_CARDAPIO_RESPOSTA, user.uid, TIMEOUT_CARDAPIO_MS, json)) {
    return "";
  }
  return jsonCampo(json, "link");
}
