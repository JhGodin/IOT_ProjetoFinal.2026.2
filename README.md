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

## Produtos, cardápio e registro de consumo (Gabriel, semanas 1 e 2)

> Observação: as seções acima ("Arquivos", "Configuracao local", "Estado atual",
> "Compilacao do teste Windows") descrevem a versão antiga do servidor em C++.
> O servidor atual é o `src/sites.py` (Flask), descrito abaixo.

### Banco de dados

O servidor usa três tabelas do PostgreSQL do grupo, criadas no Beekeeper Studio.
O `banco/tabelas.sql` não cria tabelas: documenta as colunas esperadas e traz as consultas usadas.

- `clientes`: id, nome, cpf, rfid, link, email, celular.
- `produtos`: codigo, nome, preco (em centavos, como no Projeto 04: 2500 = R$ 25,00).
- `consumos`: uma linha por produto pedido - cpf, codigo_produto, quantidade e preco no
  momento da compra (id e data_hora automáticos).

Ao ligar, o servidor confere essas tabelas e mostra no terminal `Tabela ...: ok` ou o erro.

### Rotas do `src/sites.py`

Produtos (semana 1), só para o organizador:

- `GET /produtos`: pede o login do organizador (o mesmo usuário e senha de `/avisos`,
  `ADMIN_USUARIO` e `ADMIN_SENHA` do `credenciais.py`) e lista os produtos com código e preço.
- `POST /produtos`: cadastra um produto (nome e preço em reais, como `25,90`, guardado em
  centavos) ou remove um existente. Não é preciso fazer INSERT no banco.

Consumo (semana 2):

- `GET /loja?id=...`: confere o `id` do link do cliente (coluna `link`), busca os produtos
  no banco e mostra `templates/loja.html` com nome e preço.
- `POST /loja?id=...`: lê as quantidades (`item1`, `item2`, ... = código do produto), valida
  (quantidade não numérica/negativa ou pedido vazio), grava em `consumos` e mostra
  `templates/pedido_confirmado.html` com o total do pedido e o total já consumido pelo cliente.
- Erros do pedido aparecem em `templates/pedido_invalido.html`.

O link gerado no cadastro usa o endereço em que o servidor foi aberto (por exemplo
`http://localhost:5000/loja?id=...`), não mais o IP fixo do ESP. Para clientes cadastrados
antes dessa mudança, o `banco/tabelas.sql` tem um `UPDATE` que corrige os links antigos.

### Estilo único (`src/static/estilo.css`)

O CSS que se repetia em cada página (cards, botões, formulário, controles de
quantidade, tabela) fica em `src/static/estilo.css`. Cada página do Flask importa com:

```html
<link rel="stylesheet" href="{{ url_for('static', filename='estilo.css') }}">
```

O Flask serve a pasta `static` sozinho. Já usam: `loja.html`, `pedido_confirmado.html`,
`pedido_invalido.html` e `produtos.html`. As demais páginas podem trocar o `<style>` por essa linha.
O `templates/cardapio.html` original continua igual (é o usado pelo `ESP_cardapio.cpp`);
o `loja.html` é a mesma página, mas com os itens vindos do banco.

### Como testar

1. Copiar `exemplos/credenciais.exemplo.py` para `src/credenciais/credenciais.py` e preencher
   (banco, MQTT e `ADMIN_USUARIO`/`ADMIN_SENHA`).
2. Dentro da pasta `src`, rodar `python sites.py`. O terminal deve mostrar `Tabela clientes: ok`,
   `Tabela produtos: ok` e `Tabela consumos: ok`.
3. Abrir `http://localhost:5000/produtos`, entrar como organizador e cadastrar alguns produtos.
4. Fazer um cadastro em `http://localhost:5000/` (nome e CPF válido).
5. No Beekeeper, rodar `SELECT nome, link FROM clientes;` e abrir o link do cliente no navegador.
6. Escolher os itens, confirmar e conferir as linhas novas em `consumos`.
