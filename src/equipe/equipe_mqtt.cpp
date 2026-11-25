#include "equipe_mqtt.h"

#include <ArduinoJson.h>
#include <MQTT.h>
#include <WiFi.h>

#include "../credenciais/credenciais.h"

// Sem TLS: o broker e o servidor (sites.py) também conversam em texto puro
// na porta 1883. certificados.h existe mas não é usado por ninguém hoje.
static WiFiClient rede;
static MQTTClient mqtt(1000);
static char topicoPaymentQrCode[40] = "";

// Papel escolhido pela tela de configuração, não mais por TOTEM_PAPEL: dois
// totens com o mesmo firmware podem acabar com papéis diferentes.
static char clienteId[24] = "totem-indefinido";

void equipeMqttDefinirPapel(const char *papel)
{
  snprintf(clienteId, sizeof(clienteId), "totem-%s", papel);
}

// Um slot por tópico de resposta assinado — são poucos e fixos, não precisa
// de um mapa de verdade.
struct SlotResposta
{
  const char *topico;
  String payload;
  bool novo = false;
};

static SlotResposta slots[] = {
    {TOPICO_USUARIO_RESPOSTA, "", false},
    {TOPICO_PIX_RESPOSTA, "", false},
    {TOPICO_PAGAMENTO_RESPOSTA, "", false},
    {TOPICO_CARDAPIO_RESPOSTA, "", false},
    {TOPICO_PALCO_SHOWS, "", false},
    {topicoPaymentQrCode, "", false},
};
static const size_t N_SLOTS = sizeof(slots) / sizeof(slots[0]);

static void aoChegarMensagem(String topico, String payload)
{
  for (size_t i = 0; i < N_SLOTS; i++)
  {
    if (topico == slots[i].topico)
    {
      slots[i].payload = payload;
      slots[i].novo = true;
      return;
    }
  }
}

void equipeMqttBegin()
{
  WiFi.begin(WIFI_NOME, WIFI_SENHA);
  mqtt.begin("mqtt.janks.dev.br", 1883, rede);
  mqtt.onMessage(aoChegarMensagem);
}

void equipeMqttLoop()
{
  // So pra depuracao em campo: avisa uma vez quando cada camada conecta, pra
  // diferenciar "nao pegou wifi" de "pegou wifi mas nao conectou no broker".
  static bool avisouWifi = false;
  static bool avisouMqtt = false;

  if (WiFi.status() != WL_CONNECTED)
    return;
  if (!avisouWifi)
  {
    Serial.println("WiFi conectado, IP " + WiFi.localIP().toString());
    avisouWifi = true;
  }

  if (!mqtt.connected())
  {
    // Uma tentativa por volta do loop — nunca bloqueia esperando conectar.
    if (mqtt.connect(clienteId, MQTT_USUARIO, MQTT_SENHA))
    {
      for (size_t i = 0; i < N_SLOTS; i++)
      {
        if (slots[i].topico[0] != '\0')
          mqtt.subscribe(slots[i].topico);
      }
    }
    else
    {
      Serial.println("MQTT: connect() falhou, erro " + String(mqtt.lastError()));
    }
    return;
  }

  if (!avisouMqtt)
  {
    Serial.println("MQTT conectado em mqtt.janks.dev.br");
    avisouMqtt = true;
  }

  mqtt.loop();
}

bool equipeMqttEnviarPedidoPix(const String &value, uint64_t cpfCnpj, int idTotem)
{
  snprintf(topicoPaymentQrCode, sizeof(topicoPaymentQrCode),
           "paymentQrCode/%d", idTotem);

  if (!equipeMqttPronto())
    return false;

  for (size_t i = 0; i < N_SLOTS; i++)
  {
    if (slots[i].topico == topicoPaymentQrCode)
    {
      slots[i].payload = "";
      slots[i].novo = false;
      break;
    }
  }

  if (!mqtt.subscribe(topicoPaymentQrCode))
    return false;

  JsonDocument doc;
  doc["value"] = value.toFloat();
  doc["dueDate"] = "2027-10-29";
  doc["id_totem"] = idTotem;

  JsonObject customer = doc["customer"].to<JsonObject>();
  customer["name"] = "Cliente Teste";

  char cpfCnpjTexto[21];
  snprintf(cpfCnpjTexto, sizeof(cpfCnpjTexto), "%llu",
           static_cast<unsigned long long>(cpfCnpj));
  customer["cpfCnpj"] = cpfCnpjTexto;

  String json;
  serializeJson(doc, json);

  return equipeMqttPublicar(TOPICO_PIX_PEDIDO, json);
}

bool equipeMqttAguardarQrCode(int idTotem, uint32_t timeoutMs, String &qrCode)
{
  char topicoEsperado[40];
  snprintf(topicoEsperado, sizeof(topicoEsperado),
           "paymentQrCode/%d", idTotem);

  if (strcmp(topicoPaymentQrCode, topicoEsperado) != 0)
  {
    snprintf(topicoPaymentQrCode, sizeof(topicoPaymentQrCode),
             "%s", topicoEsperado);
  }

  if (!equipeMqttPronto())
    return false;

  if (!mqtt.subscribe(topicoPaymentQrCode))
    return false;

  const uint32_t comeco = millis();
  while (millis() - comeco < timeoutMs)
  {
    equipeMqttLoop();

    if (equipeMqttResposta(topicoPaymentQrCode, qrCode))
      return true;

    delay(50);
  }

  return false;
}

bool equipeMqttPronto()
{
  return WiFi.status() == WL_CONNECTED && mqtt.connected();
}

bool equipeMqttPublicar(const char *topico, const String &payload)
{
  if (!equipeMqttPronto())
    return false;
  return mqtt.publish(topico, payload);
}

bool equipeMqttPedir(const char *topicoPedido, const String &uid)
{
  return equipeMqttPublicar(topicoPedido, "{\"uid\":\"" + uid + "\"}");
}

bool equipeMqttResposta(const char *topicoResposta, String &jsonRecebido)
{
  for (size_t i = 0; i < N_SLOTS; i++)
  {
    if (strcmp(topicoResposta, slots[i].topico) != 0)
      continue;
    if (!slots[i].novo)
      return false;
    jsonRecebido = slots[i].payload;
    slots[i].novo = false;
    return true;
  }
  return false;
}

bool equipeMqttAguardar(const char *topicoResposta, const String &uid,
                        uint32_t timeoutMs, String &jsonRecebido)
{
  const uint32_t comeco = millis();
  while (millis() - comeco < timeoutMs)
  {
    equipeMqttLoop();

    String json;
    if (equipeMqttResposta(topicoResposta, json) && jsonCampo(json, "uid") == uid)
    {
      jsonRecebido = json;
      return true;
    }

    delay(50);
  }
  return false;
}

String jsonCampo(const String &json, const String &chave)
{
  const String marcador = "\"" + chave + "\":";
  int inicio = json.indexOf(marcador);
  if (inicio < 0)
    return "";
  inicio += marcador.length();

  if (json[inicio] == '"')
  {
    int fim = json.indexOf('"', inicio + 1);
    if (fim < 0)
      return "";
    return json.substring(inicio + 1, fim);
  }

  // número ou booleano cru: termina em ',' ou '}'.
  int fim = inicio;
  while (fim < (int)json.length() && json[fim] != ',' && json[fim] != '}')
    fim++;
  String valor = json.substring(inicio, fim);
  valor.trim();
  return valor;
}
