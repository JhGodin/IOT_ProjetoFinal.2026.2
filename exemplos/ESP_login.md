# ESP_login: RFID e envio MQTT

Implementacao inicial baseada no envio MQTT de `pdf/Resumo08.pdf` e na
leitura do MFRC522 ja usada em `src/ESP_cardapio.cpp`.

## Preparacao

- Configure a placa ESP32 e instale as bibliotecas **MQTT (256dpi)** e
  **MFRC522**. WiFi e SPI fazem parte do core Arduino do ESP32.
- Copie `exemplos/credenciais.exemplo.h` para
  `src/credenciais/credenciais.h` e preencha os valores locais. Use um
  `MQTT_CLIENTE_ID` exclusivo para este ESP.
- Compile `src/ESP_login.cpp` como firmware separado. Nao inclua
  `ESP_cardapio.cpp` na mesma compilacao: ambos possuem `setup()` e `loop()`.
- Confira os pinos em `src/credenciais/pinos.h`: CS/SDA do MFRC522 em 46,
  RST em 17, e SPI padrao da placa. Para o ESP32-S3 previsto no projeto,
  SCK=12, MOSI=11 e MISO=13. Alimente o leitor em 3,3 V e conecte o GND.
  Os pinos ainda dependem da confirmacao da montagem fisica.

## Comportamento e verificacao na placa

Abra o monitor serial em **115200 baud**. A placa inicia o leitor e tenta
conectar ao WiFi e ao broker `mqtt.janks.dev.br:1883`, sem TLS, seguindo o
exemplo do PDF. Nao ha assinatura de topicos nem callback de recebimento.

Em um cliente MQTT separado, assine `totem/id`. Aproxime uma tag: o UID
deve aparecer na serial e no topico como texto hexadecimal em maiusculas,
com espacos e zeros a esquerda, por exemplo `04 0A B2 1C`.
Retire e reaproxime a tag para realizar outra leitura.

O envio usa QoS 0 e retain=false. O retorno de sucesso indica envio pelo
cliente, sem confirmacao de processamento pelo destino. Se a conexao cair,
uma leitura fica pendente em RAM e o ESP tenta envia-la apos reconectar;
enquanto isso, nao le outra tag. Reiniciar a placa perde essa pendencia.

Esta etapa apenas le o UID e o publica. O vinculo ao cadastro/CPF e a
gravacao de dados na tag ainda nao fazem parte deste firmware.
