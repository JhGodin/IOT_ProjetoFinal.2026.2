#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
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
static DNSServer dns;
static Papel      papelEscolhido = Papel::Indefinido;
static const char *papelTexto = "";

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
  dns.stop();
  WiFi.softAPdisconnect(true);
  // softAPdisconnect() desliga o AP de forma assincrona; trocar pra WIFI_STA
  // direto corre com esse desligamento ainda em andamento e o registro do
  // netif da STA falha (ESP_ERR_WIFI_STOP_STATE). WIFI_OFF entre os dois
  // garante que o radio esta mesmo parado antes de subir a STA.
  WiFi.mode(WIFI_OFF);
  delay(200);
  WiFi.mode(WIFI_STA);
  delay(300);   // deixa a fonte assentar antes do refresh do e-ink

  switch (papelEscolhido) {
    case Papel::Entrada: papelTexto = "entrada"; totemEntradaBegin(); break;
    case Papel::Saida:   papelTexto = "saida";   totemSaidaBegin();   break;
    case Papel::Comida:  papelTexto = "comida";  totemComidaBegin();  break;
    case Papel::Palco:   papelTexto = "palco";   totemPalcoBegin();   break;
    case Papel::Indefinido: break;
  }

  equipeMqttDefinirPapel(papelTexto);
  equipeMqttBegin();
}

/// Avisa o sites.py que este totem existe, pra ele gravar em "totens".
/// Só dá pra publicar depois que o MQTT conecta (equipeMqttLoop, no loop()),
/// então isso só roda uma vez, na primeira volta em que equipeMqttPronto().
static void anunciaTotemUmaVez() {
  static bool anunciado = false;
  if (anunciado || !equipeMqttPronto()) return;

  // "id" é só de enfeite (o projeto só tem um totem de cada tipo por vez) --
  // quem o sites.py usa pra não duplicar linha é o "tipo".
  String payload = "{\"id\":1,\"tipo\":\"" + String(papelTexto) + "\"}";
  equipeMqttPublicar(TOPICO_TOTEM_REGISTRO, payload);
  anunciado = true;
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

  // Portal cativo: DNS responde qualquer dominio com o IP do totem, e
  // qualquer caminho desconhecido cai na mesma pagina -- e assim que o
  // celular abre o navegador sozinho ao entrar na rede, sem precisar digitar
  // o IP. Sem isso continua funcionando igual, só sem o popup automatico.
  dns.start(53, "*", WiFi.softAPIP());

  telaBootConfiguracao(ssid);

  servidor.on("/", HTTP_GET, enviaPaginaBoot);
  servidor.on("/boot_esp.html", HTTP_GET, enviaPaginaBoot);
  servidor.on("/boot", HTTP_POST, recebeEscolhaTotem);
  servidor.onNotFound(enviaPaginaBoot);
  servidor.begin();
}

void loop() {
  if (papelEscolhido == Papel::Indefinido) {
    dns.processNextRequest();
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
  anunciaTotemUmaVez();

  switch (papelEscolhido) {
    case Papel::Entrada: totemEntradaLoop(); break;
    case Papel::Saida:   totemSaidaLoop();   break;
    case Papel::Comida:  totemComidaLoop();  break;
    case Papel::Palco:   totemPalcoLoop();   break;
    case Papel::Indefinido: break;
  }
}
