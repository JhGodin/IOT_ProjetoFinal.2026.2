#include "aviso_servidor.h"

#include <MQTT.h>
#include <WiFi.h>

#include "../aviso.h"

// PLACEHOLDER: credenciais reais saem com a equipe do WiFi/servidor.
static const char WIFI_SSID[]  = "ssid-do-evento";
static const char WIFI_SENHA[] = "senha-do-evento";

static WiFiClient  rede;
static MQTTClient  mqtt(1000);

// Payload: "titulo\ncorpo". Vazio = servidor cancelou o aviso.
static void aoChegarMensagem(String topico, String conteudo) {
  (void)topico;

  if (conteudo.length() == 0) {
    avisoLimpa();
    return;
  }

  int quebra = conteudo.indexOf('\n');
  if (quebra < 0) return;

  avisoMostra(conteudo.substring(0, quebra), conteudo.substring(quebra + 1));
}

void avisoServidorBegin() {
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  mqtt.begin("mqtt.janks.dev.br", 1883, rede);
  mqtt.onMessage(aoChegarMensagem);
}

void avisoServidorLoop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqtt.connected()) {
    if (mqtt.connect("totem")) {
      mqtt.subscribe("totem/aviso");
    }
    return;
  }

  mqtt.loop();
}
