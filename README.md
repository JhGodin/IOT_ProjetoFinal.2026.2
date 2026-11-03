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

## Semana 2 - Cardápio e registro de consumo (Gabriel)

> Observação: as seções acima ("Arquivos", "Configuracao local", "Estado atual",
> "Compilacao do teste Windows") descrevem a versão antiga do servidor em C++.
> O servidor atual é o `src/sites.py` (Flask), descrito abaixo.

### Banco de dados

O script `banco/tabelas.sql` cria as duas tabelas novas (rodar uma vez no
Beekeeper Studio) e documenta as consultas usadas:

- `_totem_ProjetoFinal` (já existia): usuários - nome, cpf, link, rfid.
- `_totem_produtos`: cardápio - codigo, nome, preco (em centavos, como no Projeto 04).
- `_totem_consumos`: uma linha por produto pedido - id e data_hora automáticos,
  cpf, codigo_produto, quantidade e preco no momento da compra.

### Rotas novas no `src/sites.py`

- `GET /loja?id=...`: confere o `id` do link gerado no cadastro (coluna `link`),
  busca os produtos no banco e mostra `templates/loja.html` com nome e preço.
- `POST /loja?id=...`: lê as quantidades (`item1`, `item2`, ... = código do produto),
  valida (quantidade não numérica/negativa ou pedido vazio), grava em
  `_totem_consumos` e mostra `templates/pedido_confirmado.html` com o total do
  pedido e o total já consumido pelo usuário no festival.
- Erros aparecem em `templates/pedido_invalido.html`.

O `templates/cardapio.html` original continua igual (é o usado pelo
`ESP_cardapio.cpp`). O `loja.html` é a mesma página, mas com os itens vindos do banco.

### Como testar

1. Rodar `banco/tabelas.sql` no banco do grupo.
2. Rodar `python src/sites.py` e fazer um cadastro em `http://localhost:5000/`.
3. Copiar o `id` da coluna `link` do usuário e abrir
   `http://localhost:5000/loja?id=<id>`.
4. Escolher os itens, confirmar e conferir as linhas novas em `_totem_consumos`.

Pendente com o grupo: o link gerado no cadastro ainda aponta para
`http://192.168.4.1/loja` (ESP). Se a loja ficar no servidor Flask, basta trocar
o começo do link pelo endereço do computador que roda o `sites.py`.
