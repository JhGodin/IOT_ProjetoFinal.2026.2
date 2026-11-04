from secrets import token_urlsafe
from threading import Thread

import paho.mqtt.client as mqtt_python  # MQTT: apenas recebimento.

from flask import Flask, request, redirect, render_template, session
from sqlalchemy import URL, create_engine, text
from sqlalchemy.exc import IntegrityError, SQLAlchemyError
from credenciais import credenciais
# MQTT: constantes importadas diretamente do arquivo Python de credenciais.
from credenciais.credenciais import MQTT_CLIENTE_ID, MQTT_USUARIO, MQTT_SENHA
# ///////nebu/////// credenciais do organizador, para a pagina /avisos
from credenciais.credenciais import ADMIN_USUARIO, ADMIN_SENHA
from validate_docbr import CPF

#Verificar o nome da tabela [_totem_]

BD = URL.create(
    drivername=credenciais.BD_TIPO, host=credenciais.BD_HOST, port=credenciais.BD_PORTA,
    database=credenciais.BD_NOME, username=credenciais.BD_USUARIO, password=credenciais.BD_SENHA,
) if credenciais.BD_TIPO else None

engine = create_engine(BD) if BD else None
if engine is None:
    raise RuntimeError("ERRO: Banco de dados não configurado.")
with engine.connect():
    pass



mqtt = mqtt_python.Client(
    callback_api_version=mqtt_python.CallbackAPIVersion.VERSION2,
    client_id=MQTT_CLIENTE_ID,
    reconnect_on_failure=True,
)
mqtt.username_pw_set(MQTT_USUARIO, MQTT_SENHA)

# RFID: ultima leitura recebida pelo ESP.
leituraRFID = None

# ///////nebu/////// ultimo aviso publicado, para mostrar na pagina /avisos
avisoAtual = None

def ConectarMQTT(cliente, dados_usuario, flags, codigo, propriedades):
    if codigo.is_failure:
        print(f"Conexao MQTT recusada: {codigo}", flush=True)
        return

    resultado, _ = cliente.subscribe("totem/rfid", qos=0)
    if resultado != mqtt_python.MQTT_ERR_SUCCESS:
        print(f"Falha ao assinar totem/rfid: {resultado}", flush=True)
def recebeuMensagem(cliente, dados_usuario, mensagem):
    global leituraRFID
    if mensagem.topic != "totem/rfid" or mensagem.retain:
        return
    leituraRFID = mensagem.payload.decode("utf-8", errors="replace").strip() or None
def reconectarMQTT():
    mqtt.loop_forever(retry_first_connection=True)

mqtt.on_connect = ConectarMQTT
mqtt.on_message = recebeuMensagem
mqtt.reconnect_delay_set(min_delay=1, max_delay=1)
mqtt.connect_async("mqtt.janks.dev.br", 1883, 10)

threadMQTT = Thread(target=reconectarMQTT, daemon=True)
threadMQTT.start()



app = Flask(__name__)
app.secret_key = token_urlsafe(32)

def tratarDados():

    cpfUsuario = request.form.get("cpf", "")
    if not CPF().validate(cpfUsuario):
        return render_template("cpf_invalido.html"), 400

    consulta = text('SELECT cpf FROM "Totem_Consumidores" WHERE cpf = :cpf')

    try:
        with engine.begin() as conexao:
            if conexao.execute(consulta, {"cpf": cpfUsuario}).first() is not None:
                return render_template("cpf_ja_registrado.html"), 409

            nomeUsuario = request.form.get("nome", "").strip()
            link = "http://192.168.4.1/loja?id=" + token_urlsafe(32) + cpfUsuario
            comando = text(
                'INSERT INTO "Totem_Consumidores" (nome, cpf, link) '
                "VALUES (:nome, :cpf, :link)"
            )
            conexao.execute(comando, {"nome": nomeUsuario, "cpf": cpfUsuario, "link": link})
    
    except IntegrityError:
        return "Nao foi possivel gravar o cadastro no banco de dados.", 500
    except SQLAlchemyError:
        return "Nao foi possivel gravar o cadastro no banco de dados.", 500

    session["cpf"] = cpfUsuario
    session.pop("rfid", None)
    return redirect("/rfid", code=303)
def tratarDadosRfid():
    global leituraRFID

    if "cpf" not in session or "link" not in session:
        return {"estado": "erro", "mensagem": "Faca o login novamente."}, 401
    
    if session.get("rfid"):
        return {"estado": "sucesso", "mensagem": "RFID registrado com sucesso."}
    
    if leituraRFID is None:
        return {"estado": "aguardando"}

    uid = leituraRFID
    try:
        with engine.begin() as conexao:
            consulta = text('SELECT cpf FROM "Totem_Consumidores" WHERE rfid = :rfid')
            
            if conexao.execute(consulta, {"rfid": uid}) is not None:
                leituraRFID = None
                return {"estado": "erro", "mensagem": "RFID ja cadastrado. Aproxime outro cartao."}, 409

            resultado = conexao.execute(
                text('UPDATE "Totem_Consumidores" SET rfid = :rfid '
                     'WHERE cpf = :cpf'),
                {"rfid": uid, "cpf": session["cpf"]},
            )
            
    except IntegrityError:
        leituraRFID = None
        return {"estado": "erro", "mensagem": "Nao foi possivel registrar este RFID. Tente outro cartao."}, 409
    except SQLAlchemyError:
        return {"estado": "erro", "mensagem": "Erro ao gravar RFID no banco. Tentando novamente..."}, 500

    session["rfid"] = uid
    leituraRFID = None
    return {"estado": "sucesso", "mensagem": "RFID registrado com sucesso."}

# ///////nebu/////// login do organizador e publicacao/cancelamento do aviso
def tratarLoginAdmin():
    if request.form.get("usuario") != ADMIN_USUARIO or request.form.get("senha") != ADMIN_SENHA:
        return redirect("/avisos", code=303)

    session["admin"] = True
    return redirect("/avisos", code=303)
def tratarAviso():
    global avisoAtual

    if request.form.get("acao") == "limpar":
        mqtt.publish("totem/aviso", payload="", qos=1, retain=True)
        avisoAtual = None
        return redirect("/avisos", code=303)

    titulo = request.form.get("titulo", "").strip()
    corpo = request.form.get("corpo", "").strip()
    mqtt.publish("totem/aviso", payload=f"{titulo}\n{corpo}", qos=1, retain=True)
    avisoAtual = {"titulo": titulo, "corpo": corpo}
    return redirect("/avisos", code=303)


@app.route("/",  methods=["GET"])
def inicio():
    return render_template("login.html")
@app.route("/", methods=["POST"])
def receberLogin():
    return tratarDados()
@app.route("/rfid", methods=["GET", "POST"])
def consultarRFID():
    if request.method == "POST":
        return tratarDadosRfid()
    if "cpf" not in session:
        return redirect("/")
    return render_template("registro_rfid.html")
@app.route("/cancel", methods=["POST"])
def cancelarCadastro():
    global leituraRFID

    if "cpf" not in session:
        return redirect("/", code=303)
    
    try:
        with engine.begin() as conexao:
            conexao.execute(
                text('DELETE FROM "Totem_Consumidores" WHERE cpf = :cpf'),
                {"cpf": session["cpf"]},
            )
    except SQLAlchemyError:
        return "Nao foi possivel cancelar o cadastro. Tente novamente.", 500
    
    session.pop("cpf", None)
    session.pop("rfid", None)
    leituraRFID = None
    return redirect("/", code=303)

# ///////nebu/////// pagina do organizador para emitir avisos (exige login)
@app.route("/avisos", methods=["GET", "POST"])
def avisos():
    if not session.get("admin"):
        if request.method == "POST":
            return tratarLoginAdmin()
        return render_template("admin_login.html")

    if request.method == "POST":
        return tratarAviso()
    return render_template("avisos.html", aviso=avisoAtual)
@app.route("/avisos/sair", methods=["POST"])
def sairAdmin():
    session.pop("admin", None)
    return redirect("/avisos", code=303)


# ///////gabriel/////// semana 2: cardapio vindo do banco (/loja) e registro de consumo
def formatarReais(centavos):
    # 2500 centavos -> "R$ 25,00"
    return f"R$ {centavos / 100:.2f}".replace(".", ",")
def buscarProdutos():
    with engine.connect() as conexao:
        linhas = conexao.execute(
            text('SELECT codigo, nome, preco FROM "Totem_Produtos" ORDER BY codigo')
        ).all()

    produtos = []
    for linha in linhas:
        produtos.append({
            "codigo": linha.codigo,
            "nome": linha.nome,
            "preco": linha.preco,
            "precoTexto": formatarReais(linha.preco),
        })
    return produtos
def buscarUsuarioDoLink(idLink):
    # O id do link e o token + CPF (11 digitos), entao o CPF sao os 11 ultimos caracteres.
    cpf = idLink[-11:]
    with engine.connect() as conexao:
        usuario = conexao.execute(
            text('SELECT nome, cpf, link FROM "Totem_Consumidores" WHERE cpf = :cpf'),
            {"cpf": cpf},
        ).first()

    if usuario is None or not usuario.link.endswith("id=" + idLink):
        return None
    return usuario
def calcularDivida(cpf):
    with engine.connect() as conexao:
        linha = conexao.execute(
            text('SELECT SUM(quantidade * preco) FROM "Totem_Consumos" WHERE cpf = :cpf'),
            {"cpf": cpf},
        ).first()

    if linha[0] is None:
        return 0
    return linha[0]
def tratarPedido():
    idLink = request.args.get("id", "")
    voltar = "/loja?id=" + idLink

    try:
        usuario = buscarUsuarioDoLink(idLink)
        if usuario is None:
            return render_template("pedido_invalido.html", mensagem="Link de cardápio inválido.", voltar=None), 404

        produtos = buscarProdutos()
        itensPedido = []
        totalPedido = 0
        for produto in produtos:
            quantidade = request.form.get("item" + str(produto["codigo"]), "0").strip()
            if not quantidade.isdigit():
                return render_template("pedido_invalido.html", mensagem="Quantidade inválida.", voltar=voltar), 400

            quantidade = int(quantidade)
            if quantidade > 0:
                subtotal = quantidade * produto["preco"]
                itensPedido.append({
                    "codigo": produto["codigo"],
                    "nome": produto["nome"],
                    "quantidade": quantidade,
                    "preco": produto["preco"],
                    "precoTexto": produto["precoTexto"],
                    "subtotalTexto": formatarReais(subtotal),
                })
                totalPedido += subtotal

        if len(itensPedido) == 0:
            return render_template("pedido_invalido.html", mensagem="Nenhum item foi escolhido.", voltar=voltar), 400

        with engine.begin() as conexao:
            for item in itensPedido:
                conexao.execute(
                    text('INSERT INTO "Totem_Consumos" (cpf, codigo_produto, quantidade, preco) '
                         "VALUES (:cpf, :codigo, :quantidade, :preco)"),
                    {"cpf": usuario.cpf, "codigo": item["codigo"],
                     "quantidade": item["quantidade"], "preco": item["preco"]},
                )

        divida = calcularDivida(usuario.cpf)

    except SQLAlchemyError:
        return "Nao foi possivel registrar o pedido no banco de dados.", 500

    return render_template(
        "pedido_confirmado.html",
        nome=usuario.nome,
        itens=itensPedido,
        total=formatarReais(totalPedido),
        divida=formatarReais(divida),
        voltar=voltar,
    )


@app.route("/loja", methods=["GET"])
def mostrarCardapio():
    idLink = request.args.get("id", "")
    try:
        usuario = buscarUsuarioDoLink(idLink)
        if usuario is None:
            return render_template("pedido_invalido.html", mensagem="Link de cardápio inválido.", voltar=None), 404
        produtos = buscarProdutos()
    except SQLAlchemyError:
        return "Nao foi possivel acessar o banco de dados.", 500

    return render_template("loja.html", nome=usuario.nome, produtos=produtos)
@app.route("/loja", methods=["POST"])
def receberPedido():
    return tratarPedido()


if __name__ == "__main__":
    try:
        app.run(debug=True, port=5000, use_reloader=False)
    finally:
        mqtt.disconnect()
        threadMQTT.join(timeout=2)
