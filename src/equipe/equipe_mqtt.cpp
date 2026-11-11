#include "equipe_mqtt.h"

#include <MQTT.h>
#include <WiFi.h>

#include "../credenciais/credenciais.h"

// Sem TLS: o broker e o servidor (sites.py) também conversam em texto puro
// na porta 1883. certificados.h existe mas não é usado por ninguém hoje.
static WiFiClient rede;
static MQTTClient mqtt(1000);

// TOTEM_PAPEL vem de build_flags (platformio.ini) como número cru — os
// nomes TOTEM_ENTRADA etc. só existem dentro de main.cpp. Mapeamento tem
// que ficar igual ao de lá: 1=entrada 2=saida 3=comida 4=palco 5=teste.
#if TOTEM_PAPEL == 1
static const char CLIENTE_ID_PAPEL[] = "totem-entrada";
#elif TOTEM_PAPEL == 2
static const char CLIENTE_ID_PAPEL[] = "totem-saida";
#elif TOTEM_PAPEL == 3
static const char CLIENTE_ID_PAPEL[] = "totem-comida";
#elif TOTEM_PAPEL == 4
static const char CLIENTE_ID_PAPEL[] = "totem-palco";
#else
static const char CLIENTE_ID_PAPEL[] = "totem-teste";
#endif

// Um slot por tópico de resposta assinado — são poucos e fixos, não precisa
// de um mapa de verdade.
struct SlotResposta {
  const char *topico;
  String      payload;
  bool        novo = false;
};

static SlotResposta slots[] = {
  {TOPICO_USUARIO_RESPOSTA, "", false},
  {TOPICO_PIX_RESPOSTA, "", false},
  {TOPICO_PAGAMENTO_RESPOSTA, "", false},
  {TOPICO_CARDAPIO_RESPOSTA, "", false},
  {TOPICO_PALCO_SHOWS, "", false},
};
static const size_t N_SLOTS = sizeof(slots) / sizeof(slots[0]);

static void aoChegarMensagem(String topico, String payload) {
  for (size_t i = 0; i < N_SLOTS; i++) {
    if (topico == slots[i].topico) {
      slots[i].payload = payload;
      slots[i].novo    = true;
      return;
    }
  }
}

void equipeMqttBegin() {
  WiFi.begin(WIFI_NOME, WIFI_SENHA);
  mqtt.begin("mqtt.janks.dev.br", 1883, rede);
  mqtt.onMessage(aoChegarMensagem);
}

void equipeMqttLoop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqtt.connected()) {
    // Uma tentativa por volta do loop — nunca bloqueia esperando conectar.
    if (mqtt.connect(CLIENTE_ID_PAPEL, MQTT_USUARIO, MQTT_SENHA)) {
      for (size_t i = 0; i < N_SLOTS; i++) mqtt.subscribe(slots[i].topico);
    }
    return;
  }

  mqtt.loop();
}

bool equipeMqttPronto() {
  return WiFi.status() == WL_CONNECTED && mqtt.connected();
}

bool equipeMqttPublicar(const char *topico, const String &payload) {
  if (!equipeMqttPronto()) return false;
  return mqtt.publish(topico, payload);
}

bool equipeMqttPedir(const char *topicoPedido, const String &uid) {
  return equipeMqttPublicar(topicoPedido, "{\"uid\":\"" + uid + "\"}");
}

bool equipeMqttResposta(const char *topicoResposta, String &jsonRecebido) {
  for (size_t i = 0; i < N_SLOTS; i++) {
    if (strcmp(topicoResposta, slots[i].topico) != 0) continue;
    if (!slots[i].novo) return false;
    jsonRecebido  = slots[i].payload;
    slots[i].novo = false;
    return true;
  }
  return false;
}

bool equipeMqttAguardar(const char *topicoResposta, const String &uid,
                         uint32_t timeoutMs, String &jsonRecebido) {
  const uint32_t comeco = millis();
  while (millis() - comeco < timeoutMs) {
    equipeMqttLoop();

    String json;
    if (equipeMqttResposta(topicoResposta, json) && jsonCampo(json, "uid") == uid) {
      jsonRecebido = json;
      return true;
    }

    delay(50);
  }
  return false;
}

String jsonCampo(const String &json, const String &chave) {
  const String marcador = "\"" + chave + "\":";
  int inicio = json.indexOf(marcador);
  if (inicio < 0) return "";
  inicio += marcador.length();

  if (json[inicio] == '"') {
    int fim = json.indexOf('"', inicio + 1);
    if (fim < 0) return "";
    return json.substring(inicio + 1, fim);
  }

  // número ou booleano cru: termina em ',' ou '}'.
  int fim = inicio;
  while (fim < (int)json.length() && json[fim] != ',' && json[fim] != '}') fim++;
  String valor = json.substring(inicio, fim);
  valor.trim();
  return valor;
}
