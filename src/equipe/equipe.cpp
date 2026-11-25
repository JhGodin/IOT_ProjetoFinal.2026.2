#include "../equipe.h"

#include <ArduinoJson.h>

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
  if (!rfidLerUID(uid)) return false;   // nenhum chip agora: caso normal, sem log

  // A partir daqui um chip FOI lido -- qualquer "false" abaixo é informação
  // nova (rede ou servidor), não mais "nada aconteceu ainda".
  Serial.println("lerUsuario: lido " + uid + ", consultando totem/usuario/pedido...");

  // Toda leitura (saída ou comida) também alimenta o fluxo de vínculo
  // UID<->CPF que o site/servidor já usa hoje (ver sites.py /rfid).
  equipeMqttPublicar(TOPICO_RFID, uid);

  if (!equipeMqttPedir(TOPICO_USUARIO_PEDIDO, uid)) {
    Serial.println("lerUsuario: sem MQTT pronto pra pedir");
    return false;
  }

  String json;
  if (!equipeMqttAguardar(TOPICO_USUARIO_RESPOSTA, uid, TIMEOUT_USUARIO_MS, json)) {
    Serial.println("lerUsuario: ninguem respondeu totem/usuario/resposta a tempo");
    return false;
  }
  if (jsonCampo(json, "erro").length() > 0) {
    Serial.println("lerUsuario: servidor respondeu erro: " + jsonCampo(json, "erro"));
    return false;
  }

  user.uid    = uid;
  user.name   = jsonCampo(json, "nome");
  user.cpfCnpj = jsonCampo(json, "cpfCnpj");
  if (user.cpfCnpj.length() == 0)
    user.cpfCnpj = jsonCampo(json, "cpf");
  user.divida = jsonCampo(json, "divida").toFloat();
  return true;
}

String pixCobranca(const User &user) {
  // O valor é a dívida que o servidor já devolveu em lerUsuario(); o totem só
  // pede o PIX pelo uid, quem calcula quanto cobrar é o servidor.
  String tmp;
  JsonDocument doc;
  doc["uid"]    = user.uid;
  doc["nome"]   = user.name;
  doc["divida"] = user.divida;
  serializeJson(doc, tmp);

  if (!equipeMqttPedir(TOPICO_PIX_PEDIDO, tmp)) return "";

  String json;
  if (!equipeMqttAguardar(TOPICO_PIX_RESPOSTA, user.uid, TIMEOUT_PIX_MS, json)) {
    return "";
  }
  return jsonCampo(json, "payload");
}

bool pagamentoConfirmado(const String &idPagamento) {
  return equipeMqttPagamentoConfirmado(idPagamento);
}

String linkCardapio(const User &user) {
  if (!equipeMqttPedir(TOPICO_CARDAPIO_PEDIDO, user.uid)) return "";

  String json;
  if (!equipeMqttAguardar(TOPICO_CARDAPIO_RESPOSTA, user.uid, TIMEOUT_CARDAPIO_MS, json)) {
    return "";
  }
  return jsonCampo(json, "link");
}

bool buscarComida(String &nome, String &link) {
  String uid;
  if (!rfidLerUID(uid)) return false;   // nenhum chip agora: caso normal, sem log

  Serial.println("buscarComida: lido " + uid + ", consultando totem/cardapio/pedido...");
  equipeMqttPublicar(TOPICO_RFID, uid);  // mesmo vinculo UID<->CPF que os outros alimentam

  if (!equipeMqttPedir(TOPICO_CARDAPIO_PEDIDO, uid)) {
    Serial.println("buscarComida: sem MQTT pronto pra pedir");
    return false;
  }

  String json;
  if (!equipeMqttAguardar(TOPICO_CARDAPIO_RESPOSTA, uid, TIMEOUT_CARDAPIO_MS, json)) {
    Serial.println("buscarComida: ninguem respondeu totem/cardapio/resposta a tempo");
    return false;
  }
  if (jsonCampo(json, "erro").length() > 0) {
    Serial.println("buscarComida: servidor respondeu erro: " + jsonCampo(json, "erro"));
    return false;
  }

  nome = jsonCampo(json, "nome");
  link = jsonCampo(json, "link");
  return true;
}
