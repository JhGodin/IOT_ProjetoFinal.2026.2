// [TESTE] Versao para Windows. O codigo da placa permanece em sites.cpp.
// Compilar na raiz do projeto: g++ -std=c++17 src/sites_teste.cpp -o sites_teste.exe -lws2_32
// Executar: .\sites_teste.exe | Abrir: http://localhost:8000/cadastro
// Wi-Fi e Serial nao sao simulados. O formulario ainda nao salva dados.
// O apoio foi reunido neste arquivo. As regioes podem ser recolhidas no VS Code.

#pragma region TESTE - Tipos com escrita semelhante ao Arduino
#include <winsock2.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

// [TESTE] Nomes para tipos da biblioteca padrao, usados apenas na adaptacao.
using Texto = std::string;
using AcaoPagina = std::function<void()>;
constexpr int HTTP_GET = 0;

// [TESTE] Implementa somente os recursos de String usados pelas paginas.
class String {
public:
  String() = default;
  String(const char* texto) : valor(texto) {}
  String(Texto texto) : valor(std::move(texto)) {}

  const char* c_str() const { return valor.c_str(); }
  const Texto& texto() const { return valor; }
  void replace(const String& origem, const String& destino);

private:
  Texto valor;
};

// [TESTE] File e ArquivosPC leem do computador, em vez do LittleFS da placa.
class File {
public:
  explicit File(const char* caminho) : arquivo(caminho, std::ios::binary) {}
  explicit operator bool() const { return arquivo.is_open(); }
  String readString();
  void close() { arquivo.close(); }

private:
  std::ifstream arquivo;
};

class SistemaArquivosPC {
public:
  File open(const char* caminho, const char* modo);
};
SistemaArquivosPC ArquivosPC;

// [TESTE] Interface semelhante ao WebServer do ESP32, com rotas GET livres.
class WebServer {
public:
  explicit WebServer(int numeroPorta) : porta(numeroPorta) {}
  ~WebServer();
  WebServer(const WebServer&) = delete;
  WebServer& operator=(const WebServer&) = delete;

  void on(const char* caminho, int metodo, AcaoPagina pagina);
  void send(int codigo, const char* tipo, const String& conteudo);
  void begin();
  void handleClient();

private:
  int porta;
  SOCKET conexaoServidor = INVALID_SOCKET;
  bool redeIniciada = false;
  std::map<Texto, AcaoPagina> paginas;
  int codigoResposta = 200;
  Texto tipoResposta;
  Texto corpoResposta;

  void atenderPedido(const Texto& pedido);
  void enviarResposta(SOCKET cliente);
};
#pragma endregion

#pragma region Paginas e rotas - mesma logica de sites.cpp
WebServer servidor(8000); // [TESTE] No ESP32, o servidor usa a porta 80.

String nomeUsuario;
String cpfUsuario;

void pagina1 () {
  // [TESTE] No ESP32: File arquivo = LittleFS.open("/ajustes.html", "r");
  File arquivo = ArquivosPC.open("data/ajustes.html", "r");
  if (!arquivo) {
    servidor.send(500, "text/html", "Erro no HTML");
    return;
  }
  String html = arquivo.readString();
  arquivo.close();
  html.replace("{{nome}}", nomeUsuario);
  html.replace("{{cpf}}", cpfUsuario);

  servidor.send(200, "text/html", html);
}

void pagina2 () {
  servidor.send(200, "text/html", "Bem-vindo!");
}

void setup() {
  // [TESTE] A conexao de rede e fornecida pelo Windows; nao exige resposta Serial.
  servidor.on("/cadastro", HTTP_GET, pagina1);
  servidor.on("/loja", HTTP_GET, pagina2);
  servidor.on("/", HTTP_GET, pagina1); // [TESTE] Atalho para abrir a pagina inicial.
  servidor.begin();
}

void loop() {
  servidor.handleClient();
}
#pragma endregion

#pragma region TESTE - Entrada do programa no computador
// [TESTE] No Arduino, a plataforma fornece main() e chama setup()/loop().
int main() {
  try {
    setup();
    while (true) {
      loop();
    }
  } catch (const std::exception& erro) {
    std::cerr << "[TESTE] " << erro.what() << '\n';
    return 1;
  }
}
#pragma endregion

#pragma region TESTE - Implementacao do apoio para Windows
// [TESTE] Detalhes de arquivo e HTTP. Para editar as paginas, use a regiao acima.
// Este apoio nao e uma implementacao completa das bibliotecas Arduino.

void String::replace(const String& origem, const String& destino) {
  // Copias permitem substituir todas as ocorrencias, mesmo usando a propria String.
  const Texto antes = origem.texto();
  const Texto depois = destino.texto();
  if (antes.empty()) return;

  std::size_t posicao = 0;
  while ((posicao = valor.find(antes, posicao)) != Texto::npos) {
    valor.replace(posicao, antes.size(), depois);
    posicao += depois.size();
  }
}

String File::readString() {
  return Texto(std::istreambuf_iterator<char>(arquivo), std::istreambuf_iterator<char>());
}

File SistemaArquivosPC::open(const char* caminho, const char* modo) {
  if (Texto(modo) != "r") {
    throw std::runtime_error("ArquivosPC implementa somente leitura (r).");
  }
  return File(caminho);
}

WebServer::~WebServer() {
  if (conexaoServidor != INVALID_SOCKET) closesocket(conexaoServidor);
  if (redeIniciada) WSACleanup();
}

void WebServer::on(const char* caminho, int metodo, AcaoPagina pagina) {
  if (metodo != HTTP_GET) {
    throw std::runtime_error("[TESTE] Somente GET implementado; salvamento ainda pendente.");
  }
  paginas[caminho] = std::move(pagina);
}

void WebServer::send(int codigo, const char* tipo, const String& conteudo) {
  codigoResposta = codigo;
  tipoResposta = tipo;
  corpoResposta = conteudo.texto();
}

void WebServer::begin() {
  if (redeIniciada) throw std::runtime_error("Servidor ja iniciado.");
  if (porta < 1 || porta > 65535) throw std::runtime_error("Porta invalida.");

  WSADATA dados;
  if (WSAStartup(MAKEWORD(2, 2), &dados) != 0) {
    throw std::runtime_error("Erro ao iniciar a rede do Windows.");
  }
  redeIniciada = true;
  conexaoServidor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  sockaddr_in endereco{};
  endereco.sin_family = AF_INET;
  endereco.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // [TESTE] Somente este computador.
  endereco.sin_port = htons(static_cast<unsigned short>(porta));

  if (conexaoServidor == INVALID_SOCKET ||
      bind(conexaoServidor, reinterpret_cast<sockaddr*>(&endereco), sizeof(endereco)) == SOCKET_ERROR ||
      listen(conexaoServidor, SOMAXCONN) == SOCKET_ERROR) {
    throw std::runtime_error("Nao foi possivel abrir a porta " + std::to_string(porta) +
                             ". Confira se outra versao esta rodando.");
  }

  std::cout << "[TESTE] Abra http://localhost:" << porta << "/cadastro\n"
            << "[TESTE] Leitura de data/ajustes.html. Ctrl+C para encerrar.\n";
}

void WebServer::atenderPedido(const Texto& pedido) {
  std::istringstream cabecalho(pedido);
  Texto metodo, caminho, versao;
  cabecalho >> metodo >> caminho >> versao;

  if (caminho.empty() || (versao != "HTTP/1.1" && versao != "HTTP/1.0")) {
    send(400, "text/html", "Pedido HTTP invalido");
    return;
  }
  if (metodo != "GET") {
    send(405, "text/html", "[TESTE] O formulario ainda nao possui salvamento.");
    return;
  }

  caminho = caminho.substr(0, caminho.find('?'));
  auto pagina = paginas.find(caminho);
  send(404, "text/html", "Pagina nao encontrada");
  if (pagina != paginas.end()) pagina->second();
}

void WebServer::enviarResposta(SOCKET cliente) {
  const char* motivo;
  switch (codigoResposta) {
    case 200: motivo = "OK"; break;
    case 400: motivo = "Bad Request"; break;
    case 404: motivo = "Not Found"; break;
    case 405: motivo = "Method Not Allowed"; break;
    case 431: motivo = "Request Header Fields Too Large"; break;
    case 500: motivo = "Internal Server Error"; break;
    default: motivo = "Response";
  }

  Texto resposta = "HTTP/1.1 " + std::to_string(codigoResposta) + " " + motivo +
    "\r\nContent-Type: " + tipoResposta + "; charset=utf-8\r\nContent-Length: " +
    std::to_string(corpoResposta.size()) + "\r\nConnection: close\r\n" +
    (codigoResposta == 405 ? "Allow: GET\r\n" : "") + "\r\n" + corpoResposta;

  std::size_t enviados = 0;
  while (enviados < resposta.size()) {
    int quantidade = ::send(cliente, resposta.data() + enviados,
                            static_cast<int>(resposta.size() - enviados), 0);
    if (quantidade <= 0) break;
    enviados += quantidade;
  }
}

void WebServer::handleClient() {
  if (conexaoServidor == INVALID_SOCKET) throw std::runtime_error("Chame begin() primeiro.");

  // [TESTE] Retorna ao loop quando nao ha clientes. Nao simula tempos do hardware.
  fd_set conexoes;
  FD_ZERO(&conexoes);
  FD_SET(conexaoServidor, &conexoes);
  timeval espera{0, 10000};
  int disponiveis = select(0, &conexoes, nullptr, nullptr, &espera);
  if (disponiveis == SOCKET_ERROR) throw std::runtime_error("Erro ao aguardar cliente.");
  if (disponiveis == 0) return;

  SOCKET cliente = accept(conexaoServidor, nullptr, nullptr);
  if (cliente == INVALID_SOCKET) throw std::runtime_error("Erro ao aceitar cliente.");

  try {
    DWORD timeout = 3000;
    setsockopt(cliente, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(cliente, SOL_SOCKET, SO_SNDTIMEO,
               reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    Texto pedido;
    char buffer[2048];
    while (pedido.find("\r\n\r\n") == Texto::npos && pedido.size() < 16384) {
      int recebidos = recv(cliente, buffer, sizeof(buffer), 0);
      if (recebidos <= 0) break;
      pedido.append(buffer, recebidos);
    }

    if (pedido.size() >= 16384) {
      send(431, "text/html", "Cabecalho muito grande");
      enviarResposta(cliente);
    } else if (pedido.find("\r\n\r\n") != Texto::npos) {
      atenderPedido(pedido);
      enviarResposta(cliente);
    }
  } catch (...) {
    closesocket(cliente);
    throw;
  }
  closesocket(cliente);
}
#pragma endregion