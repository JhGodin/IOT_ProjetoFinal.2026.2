# IOT_ProjetoFinal.2026.2

Projeto Final de Internet das Coisas.

Projeto de totem informativo e pagamento

## Arquivos

- `src/sites.cpp`: versao destinada ao ESP32.
- `src/sites_teste.cpp`: adaptacao para testes locais no Windows.
- `src/certificados.h`: certificados publicos usados na configuracao TLS.
- `src/credenciais.exemplo.h`: modelo de configuracao do laboratório.
- `data/login.html`: formulario para o cadastro.
- `pdf/Resumo08.pdf`: material de referencia da aula.

## Configuracao local

Copie `src/credenciais.exemplo.h` para `src/credenciais.h` e preencha os dados
de Wi-Fi e MQTT. O arquivo real de credenciais e `credenciais.txt` sao locais
e nao devem ser versionados.

## Estado atual

A versao ESP32 depende de configurar a placa, as bibliotecas e a porta TLS
do broker (`portaTLS`, ainda pendente no codigo). O formulario envia os dados
por POST e o programa monta um JSON para publicar em `totem/login`.
A gravacao local do JSON ainda nao foi implementada.

A adaptacao Windows ainda usa `/cadastro`, leitura de `data/ajustes.html` e
somente GET. Ela precisa ser sincronizada com o formulario atual, renomeado
para `data/login.html`, e com as novas rotas, POST e MQTT da versao ESP32.

## Compilacao do teste Windows

Com MinGW-w64 instalado, execute na raiz do projeto:

```powershell
g++ -std=c++17 src/sites_teste.cpp -o sites_teste.exe -lws2_32
.\sites_teste.exe
```

Os executaveis sao gerados localmente e nao sao enviados ao GitHub.

## Arquivos HTML na placa

Com o ambiente da placa configurado no PlatformIO, use as tarefas
`Build Filesystem Image` e `Upload Filesystem Image` para enviar a pasta `data`
ao LittleFS. O arquivo e acessado na placa como `/login.html`.
Ao configurar a compilacao do ESP32, exclua `src/sites_teste.cpp`, que e
especifico do Windows.
