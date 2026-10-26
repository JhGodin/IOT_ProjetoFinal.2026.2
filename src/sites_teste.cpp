// [TESTE] Versao para Windows. O codigo da placa permanece em sites.cpp.
// Compilar na raiz do projeto: g++ -std=c++17 src/sites_teste.cpp -o sites_teste.exe -lws2_32
// Executar: .\sites_teste.exe | Abrir: http://localhost:8000/login
// Wi-Fi e Serial nao sao simulados. MQTT apenas imprime no terminal.
// O apoio foi reunido neste arquivo. As regioes podem ser recolhidas no VS Code.

#pragma region TESTE - Tipos com escrita semelhante ao Arduino
#include <winsock2.h>
#include <algorithm>
#include <cctype>
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
constexpr int HTTP_POST = 1;

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

// [TESTE] Interface semelhante ao WebServer do ESP32, com GET e POST.
class WebServer {
public:
  explicit WebServer(int numeroPorta) : porta(numeroPorta) {}
  ~WebServer();
  WebServer(const WebServer&) = delete;
  WebServer& operator=(const WebServer&) = delete;

  void on(const char* caminho, int metodo, AcaoPagina pagina);
  void send(int codigo, const char* tipo = "text/html", const String& conteudo = "");
  void sendHeader(const char* nome, const char* valor);
  String arg(const char* nome);
  void begin();
  void handleClient();

private:
  int porta;
  SOCKET conexaoServidor = INVALID_SOCKET;
  bool redeIniciada = false;
  std::map<std::pair<Texto, int>, AcaoPagina> paginas;
  std::map<Texto, Texto> argumentos;
  Texto cabecalhosResposta;
  int codigoResposta = 200;
  Texto tipoResposta;
  Texto corpoResposta;

  void atenderPedido(const Texto& pedido);
  void enviarResposta(SOCKET cliente);
};
#pragma endregion

// [TESTE] Apoio minimo para manter tratarDados() igual ao codigo da placa.
// Nao substitui ArduinoJson ou MQTT: apenas monta o JSON e imprime no terminal.
using JsonDocument = std::map<Texto, String>;

Texto escaparJson(const Texto& texto) {
  Texto resultado = "\"";
  const char* hexadecimal = "0123456789abcdef";
  for (unsigned char c : texto) {
    if (c == '"' || c == '\\') resultado += '\\';
    if (c < 32) {
      resultado += "\\u00";
      resultado += hexadecimal[c >> 4];
      resultado += hexadecimal[c & 15];
    } else {
      resultado += static_cast<char>(c);
    }
  }
  return resultado + "\"";
}

void serializeJson(const JsonDocument& dados, String& textoJson) {
  Texto resultado = "{";
  for (const auto& campo : dados) {
    if (resultado.size() > 1) resultado += ",";
    resultado += escaparJson(campo.first) + ":" + escaparJson(campo.second.texto());
  }
  textoJson = resultado + "}";
}

class MQTTSimulado {
public:
  void publish(const char* topico, const String& mensagem, bool, int) {
    std::cout << "[MQTT SIMULADO] " << topico << ": " << mensagem.texto() << std::endl;
  }
};
MQTTSimulado mqtt;

#pragma region Paginas e rotas - mesma logica de sites.cpp
WebServer servidor(8000); // [TESTE] No ESP32, o servidor usa a porta 80.

String nomeUsuario;
String cpfUsuario;

void pagina1 () {
  File arquivo = ArquivosPC.open("data/login.html", "r");
  if (!arquivo) {
    servidor.send(500, "text/html", "Erro no HTML [login]");
    return;
  }
  String html = arquivo.readString();
  arquivo.close();

  servidor.send(200, "text/html", html);
}
void pagina2 () {
  File arquivo = ArquivosPC.open("data/cardapio.html", "r");
  if (!arquivo) {
    servidor.send(500, "text/html", "Erro no HTML [cardapio]");
    return;
  }
  String html = arquivo.readString();
  arquivo.close();

  servidor.send(200, "text/html", html);
}
void pagina0 () {
  servidor.send(200, "text/html", "Obrigado!!!");
}
void tratarDados() {
  JsonDocument dados;
  String textoJson;

  nomeUsuario = servidor.arg("nome");
  cpfUsuario = servidor.arg("cpf");
  dados["usuario"] = nomeUsuario;
  dados["cpf"] = cpfUsuario;

  serializeJson(dados, textoJson);
  mqtt.publish("totem/login", textoJson, false, 1);

  servidor.sendHeader("Location", "/login_realizado");
  servidor.send(303);
}



void setup() {
  // [TESTE] Windows fornece a rede; arquivos sao lidos da pasta data.
  servidor.on("/login", HTTP_GET, pagina1);
  servidor.on("/login", HTTP_POST, tratarDados);
  servidor.on("/login_realizado", HTTP_GET, pagina0);
  servidor.on("/loja", HTTP_GET, pagina2);
  servidor.on("/", HTTP_GET, pagina1); // [TESTE] Atalho local.
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
  paginas[{caminho, metodo}] = std::move(pagina);
}

void WebServer::send(int codigo, const char* tipo, const String& conteudo) {
  codigoResposta = codigo;
  tipoResposta = tipo;
  corpoResposta = conteudo.texto();
}

void WebServer::sendHeader(const char* nome, const char* valor) {
  cabecalhosResposta += Texto(nome) + ": " + valor + "\r\n";
}

String WebServer::arg(const char* nome) {
  auto campo = argumentos.find(nome);
  return campo == argumentos.end() ? String("") : String(campo->second);
}

// [TESTE] O navegador codifica espacos como + e outros bytes como %XX.
Texto decodificarFormulario(const Texto& texto) {
  Texto resultado;
  for (std::size_t i = 0; i < texto.size(); i++) {
    if (texto[i] == '+') {
      resultado += ' ';
    } else if (texto[i] == '%') {
      if (i + 2 >= texto.size() ||
          texto.find_first_not_of("0123456789abcdefABCDEF", i + 1) < i + 3) {
        throw std::runtime_error("Formulario com codificacao invalida.");
      }
      resultado += static_cast<char>(std::stoi(texto.substr(i + 1, 2), nullptr, 16));
      i += 2;
    } else {
      resultado += texto[i];
    }
  }
  return resultado;
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

  std::cout << "[TESTE] Abra http://localhost:" << porta << "/login\n"
            << "[TESTE] Paginas: /login e /loja. MQTT simulado no terminal. Ctrl+C para encerrar.\n";
}

void WebServer::atenderPedido(const Texto& pedido) {
  argumentos.clear();
  cabecalhosResposta.clear();
  std::istringstream cabecalho(pedido);
  Texto metodo, caminho, versao;
  cabecalho >> metodo >> caminho >> versao;

  if (caminho.empty() || (versao != "HTTP/1.1" && versao != "HTTP/1.0")) {
    send(400, "text/html", "Pedido HTTP invalido");
    return;
  }
  if (metodo != "GET" && metodo != "POST") {
    send(405, "text/html", "Metodo nao suportado.");
    return;
  }

  caminho = caminho.substr(0, caminho.find('?'));
  if (metodo == "POST") {
    Texto corpo = pedido.substr(pedido.find("\r\n\r\n") + 4);
    std::istringstream campos(corpo);
    Texto campo;
    try {
      while (std::getline(campos, campo, '&')) {
        auto igual = campo.find('=');
        if (igual == Texto::npos) continue;
        argumentos[decodificarFormulario(campo.substr(0, igual))] =
          decodificarFormulario(campo.substr(igual + 1));
      }
    } catch (const std::exception&) {
      send(400, "text/plain", "Formulario invalido.");
      return;
    }
  }
  auto pagina = paginas.find({caminho, metodo == "POST" ? HTTP_POST : HTTP_GET});
  send(404, "text/html", "Pagina nao encontrada");
  if (pagina != paginas.end()) {
    pagina->second();
  } else if (paginas.count({caminho, HTTP_GET}) || paginas.count({caminho, HTTP_POST})) {
    send(405, "text/plain", "Metodo nao disponivel para esta pagina.");
  }
}

void WebServer::enviarResposta(SOCKET cliente) {
  const char* motivo;
  switch (codigoResposta) {
    case 200: motivo = "OK"; break;
    case 303: motivo = "See Other"; break;
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
    cabecalhosResposta + "\r\n" + corpoResposta;

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
    cabecalhosResposta.clear(); // Cada cliente recebe apenas seus proprios cabecalhos.
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
      // [TESTE] O corpo do POST pode chegar depois dos cabecalhos.
      std::size_t inicioCorpo = pedido.find("\r\n\r\n") + 4;
      std::size_t tamanhoCorpo = 0;
      bool tamanhoValido = true;
      std::istringstream linhas(pedido.substr(0, inicioCorpo - 4));
      Texto linha;
      std::getline(linhas, linha); // Pula a primeira linha do pedido.
      while (std::getline(linhas, linha)) {
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();
        auto doisPontos = linha.find(':');
        if (doisPontos == Texto::npos) continue;
        Texto nome = linha.substr(0, doisPontos);
        std::transform(nome.begin(), nome.end(), nome.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (nome == "transfer-encoding") tamanhoValido = false;
        if (nome != "content-length") continue;
        std::istringstream valor(linha.substr(doisPontos + 1));
        Texto extra;
        if (!(valor >> tamanhoCorpo) || (valor >> extra) || tamanhoCorpo > 16384) {
          tamanhoValido = false;
        }
      }
      if (!tamanhoValido) {
        cabecalhosResposta.clear();
        send(400, "text/plain", "Tamanho de formulario invalido (limite: 16 KB).");
        enviarResposta(cliente);
        closesocket(cliente);
        return;
      }
      while (pedido.size() < inicioCorpo + tamanhoCorpo) {
        int recebidos = recv(cliente, buffer, sizeof(buffer), 0);
        if (recebidos <= 0) break;
        pedido.append(buffer, recebidos);
      }
      if (pedido.size() < inicioCorpo + tamanhoCorpo) {
        cabecalhosResposta.clear();
        send(400, "text/plain", "Formulario incompleto.");
      } else {
        pedido.resize(inicioCorpo + tamanhoCorpo);
        atenderPedido(pedido);
      }
      enviarResposta(cliente);
    }
  } catch (...) {
    closesocket(cliente);
    throw;
  }
  closesocket(cliente);
}
#pragma endregion
