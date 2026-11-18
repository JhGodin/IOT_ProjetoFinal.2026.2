#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "nebu/display.h"
#include "nebu/telas.h"
#include "nebu/totem_entrada.h"
#include "nebu/totem_saida.h"
#include "nebu/totem_comida.h"
#include "nebu/totem_palco.h"
#include "equipe/equipe_mqtt.h"
#include "equipe/equipe_rfid.h"

// ---------------------------------------------------------------------------
// Boot: o papel do totem não vem de build_flags, e sim do formulário em
// boot_esp.html. Ao ligar, o totem sobe um AP aberto e mostra o QR pra
// entrar nele; depois de escolhido o papel, o AP cai e entra na rede de
// verdade (equipeMqttBegin), e o loop() passa a chamar só aquele totem.
// ---------------------------------------------------------------------------

enum class Papel : uint8_t { Indefinido, Entrada, Saida, Comida, Palco };

static WebServer servidor(80);
static Papel      papelEscolhido = Papel::Indefinido;

static void enviaPaginaBoot() {
  File arquivo = LittleFS.open("/boot_esp.html", "r");
  if (!arquivo) {
    servidor.send(500, "text/plain", "boot_esp.html nao encontrado no LittleFS");
    return;
  }
  servidor.streamFile(arquivo, "text/html");
  arquivo.close();
}

static void recebeEscolhaTotem() {
  String escolha = servidor.arg("totem");

  if (escolha == "Entrada")      papelEscolhido = Papel::Entrada;
  else if (escolha == "Saida")   papelEscolhido = Papel::Saida;
  else if (escolha == "Comida")  papelEscolhido = Papel::Comida;
  else if (escolha == "Palco")   papelEscolhido = Papel::Palco;
  else {
    servidor.send(400, "text/plain", "Escolha invalida");
    return;
  }

  servidor.send(200, "text/html", "<p>Totem configurado. Pode fechar esta pagina.</p>");
}

/// Sai do modo de configuração: desliga o AP, entra na rede de verdade e
/// chama o Begin do papel escolhido. Só roda uma vez.
static void iniciaTotemEscolhido() {
  servidor.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  delay(300);   // deixa a fonte assentar antes do refresh do e-ink

  switch (papelEscolhido) {
    case Papel::Entrada:
      equipeMqttDefinirPapel("entrada");
      totemEntradaBegin();
      break;
    case Papel::Saida:
      equipeMqttDefinirPapel("saida");
      totemSaidaBegin();
      break;
    case Papel::Comida:
      equipeMqttDefinirPapel("comida");
      totemComidaBegin();
      break;
    case Papel::Palco:
      equipeMqttDefinirPapel("palco");
      totemPalcoBegin();
      break;
    case Papel::Indefinido:
      break;
  }

  equipeMqttBegin();
}

void setup() {
  Serial.begin(115200);
  displayBegin();
  rfidBegin();

  if (!LittleFS.begin(true)) {
    Serial.println("Falha ao montar LittleFS");
  }

  // Nome único por placa (ultimos bytes do MAC), pra várias telas ligadas ao
  // mesmo tempo não subirem APs com o mesmo nome.
  String ssid = "Totem-" + WiFi.macAddress().substring(12);
  ssid.replace(":", "");
  WiFi.softAP(ssid.c_str());   // rede aberta, sem senha

  telaBootConfiguracao(ssid);

  servidor.on("/", HTTP_GET, enviaPaginaBoot);
  servidor.on("/boot_esp.html", HTTP_GET, enviaPaginaBoot);
  servidor.on("/boot", HTTP_POST, recebeEscolhaTotem);
  servidor.begin();

}

void loop() {
  if (papelEscolhido == Papel::Indefinido) {
    Serial.println("setup loop\n");
    servidor.handleClient();
    return;
  }

  static bool jaIniciou = false;
  if (!jaIniciou) {
    Serial.println("worked\n");
    iniciaTotemEscolhido();
    jaIniciou = true;
  }

  equipeMqttLoop();

  switch (papelEscolhido) {
    case Papel::Entrada: totemEntradaLoop(); Serial.println("worked\n"); break;
    case Papel::Saida:   totemSaidaLoop();   break;
    case Papel::Comida:  totemComidaLoop();  break;
    case Papel::Palco:   totemPalcoLoop();   break;
    case Papel::Indefinido: break;
  }
}
