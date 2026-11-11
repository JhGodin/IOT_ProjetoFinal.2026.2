from secrets import token_urlsafe
from threading import Thread

import paho.mqtt.client as mqtt_python  # MQTT: apenas recebimento.

from flask import Flask, request, redirect, render_template, session
from sqlalchemy import URL, create_engine, text
from sqlalchemy.exc import IntegrityError, SQLAlchemyError
from credenciais import credenciais
# MQTT: constantes importadas diretamente do arquivo Python de credenciais.
from credenciais.credenciais import MQTT_CLIENTE_ID, MQTT_USUARIO, MQTT_SENHA
from validate_docbr import CPF

BD = URL.create(
    drivername=credenciais.BD_TIPO, host=credenciais.BD_HOST, port=credenciais.BD_PORTA,
    database=credenciais.BD_NOME, username=credenciais.BD_USUARIO, password=credenciais.BD_SENHA,
) if credenciais.BD_TIPO else None

engine = create_engine(BD) if BD else None
if engine is None:
    raise RuntimeError("ERRO: Banco de dados não configurado.")
with engine.begin() as conexao:
    conexao.execute(text('ALTER TABLE "clientes" ADD COLUMN IF NOT EXISTS telefone TEXT'))
    conexao.execute(text('ALTER TABLE "clientes" ADD COLUMN IF NOT EXISTS email TEXT'))



mqtt = mqtt_python.Client(
    callback_api_version=mqtt_python.CallbackAPIVersion.VERSION2,
    client_id=MQTT_CLIENTE_ID,
    reconnect_on_failure=True,
)
mqtt.username_pw_set(MQTT_USUARIO, MQTT_SENHA)


leituraRFID = None
cpfAtual = None


def ConectarMQTT(cliente, dados_usuario, flags, codigo, propriedades):
    if codigo.is_failure:
        print(f"Conexao MQTT recusada: {codigo}", flush=True)
        return

    resultado, _ = cliente.subscribe("totem/rfid", qos=0)
    if resultado != mqtt_python.MQTT_ERR_SUCCESS:
        print(f"Falha ao assinar totem/rfid: {resultado}", flush=True)
def reconectarMQTT():
    mqtt.loop_forever(retry_first_connection=True)
def recebeuMensagem(cliente, dados_usuario, mensagem):
    global leituraRFID
    global cpfAtual

    print(mensagem)
    if mensagem.topic != "totem/rfid" or mensagem.retain:
        return

    if cpfAtual != None:

        leituraRFID = mensagem.payload.decode("utf-8", errors="replace").strip() or None
        print(leituraRFID)

mqtt.on_connect = ConectarMQTT
mqtt.on_message = recebeuMensagem
mqtt.reconnect_delay_set(min_delay=1, max_delay=1)
mqtt.connect_async("mqtt.janks.dev.br", 1883, 10)

threadMQTT = Thread(target=reconectarMQTT, daemon=True)
threadMQTT.start()


app = Flask(__name__)
app.secret_key = token_urlsafe(32)


def tratarDados():
    global cpfAtual

    telefoneUsuario = request.form.get("telefone", "").strip()
    emailUsuario = request.form.get("email", "").strip()
    if not telefoneUsuario or not emailUsuario:
        return "Informe o telefone e o email.", 400

    cpfUsuario = request.form.get("cpf", "")
    if not CPF().validate(cpfUsuario):
        return render_template("cpf_invalido.html"), 400

    consulta = text('SELECT cpf FROM "clientes" WHERE cpf = :cpf')

    try:
        with engine.begin() as conexao:
            if conexao.execute(consulta, {"cpf": cpfUsuario}).first() is not None:
                return render_template("cpf_ja_registrado.html"), 409

            nomeUsuario = request.form.get("nome", "").strip()
            link = "http://192.168.4.1/loja?id=" + token_urlsafe(32) + cpfUsuario
           
    
    except IntegrityError:
        return "Nao foi possivel gravar o cadastro no banco de dados.", 500
    except SQLAlchemyError:
        return "Nao foi possivel gravar o cadastro no banco de dados.", 500

    session["cpf"] = cpfUsuario
    session["link"] = link
    session["nome"] = nomeUsuario
    session["telefone"] = telefoneUsuario
    session["email"] = emailUsuario
    
    cpfAtual = cpfUsuario

    return redirect("/rfid", code=303)
def tratarDadosRfid():
    global leituraRFID

    if "cpf" not in session:
        print({"estado": "erro", "mensagem": "Faca o login novamente."})
        return {"estado": "erro", "mensagem": "Faca o login novamente."}, 401
    
    if session.get("rfid"):
        print ({"estado": "sucesso", "mensagem": "RFID registrado com sucesso."})
        return {"estado": "sucesso", "mensagem": "RFID registrado com sucesso."}
    if leituraRFID is None:
        print({"estado": "aguardando"})
        return {"estado": "aguardando"}

    uid = leituraRFID
    try:
        with engine.begin() as conexao:
            consulta = text('SELECT cpf FROM "clientes" WHERE rfid = :rfid')
            
            if conexao.execute(consulta, {"rfid": uid}).first() is not None:
                leituraRFID = None
                print("409-2")
                return {"estado": "erro", "mensagem": "RFID ja cadastrado. Aproxime outro cartao."}, 409
            

            comando = text('INSERT INTO "clientes" (nome, cpf, telefone, email, link, rfid) '
            "VALUES (:nome, :cpf, :telefone, :email, :link, :rfid)")
            resultado = conexao.execute(comando, {"nome": session["nome"], "cpf": session["cpf"], "telefone": session.get("telefone", ""), "email": session.get("email", ""), "link": session["link"], "rfid": uid})

            
    except IntegrityError:
        leituraRFID = None
        print("409-3")
        return {"estado": "erro", "mensagem": "Nao foi possivel registrar este RFID. Tente outro cartao."}, 409
    except SQLAlchemyError:
        return {"estado": "erro", "mensagem": "Erro ao gravar RFID no banco. Tentando novamente..."}, 500

    session["rfid"] = uid
    leituraRFID = None
    return {"estado": "sucesso", "mensagem": "RFID registrado com sucesso."}


@app.route("/editar", methods=["GET"])
def editar():
    usuariosPorPagina = 10
    pagina = max(1, request.args.get("pagina", default=1, type=int))

    try:
        with engine.connect() as conexao:
            totalUsuarios = conexao.execute(text('SELECT COUNT(*) FROM "clientes"')).scalar_one()
            totalPaginas = max(1, (totalUsuarios + usuariosPorPagina - 1) // usuariosPorPagina)
            pagina = min(pagina, totalPaginas)
            usuarios = conexao.execute(
                text('SELECT id, nome, cpf FROM "clientes" ORDER BY nome, cpf LIMIT :limite OFFSET :inicio'),
                {"limite": usuariosPorPagina, "inicio": (pagina - 1) * usuariosPorPagina},
            ).mappings().all()
    except SQLAlchemyError:
        return "Nao foi possivel listar os usuarios. Tente novamente.", 500

    return render_template(
        "editar.html", usuarios=usuarios, pagina=pagina, totalPaginas=totalPaginas,
    )
@app.route("/editar/<cpf>", methods=["GET", "POST"])
def editarUsuario(cpf):
    try:
        with engine.begin() as conexao:
            usuario = conexao.execute(
                text('SELECT nome, cpf, telefone, email FROM "clientes" WHERE cpf = :cpf'),
                {"cpf": cpf},
            ).mappings().first()

            if usuario is None:
                return "Usuario nao encontrado.", 404

            if request.method == "POST":
                dados = {campo: request.form.get(campo, "").strip()
                         for campo in ("nome", "cpf", "telefone", "email")}
                
                if not dados["nome"]:
                    return "Informe o nome.", 400
                if not dados["telefone"]:
                                    return "Informe o telefone.", 400
                if not dados["email"]:
                                    return "Informe o email.", 400
                if not CPF().validate(dados["cpf"]):
                    return render_template("cpf_invalido.html"), 400

                dados["cpf_original"] = cpf
                conexao.execute(
                    text('UPDATE "clientes" SET nome = :nome, cpf = :cpf, '
                         'telefone = :telefone, email = :email WHERE cpf = :cpf_original'),
                    dados,
                )
    except IntegrityError:
        return "Nao foi possivel salvar. Verifique se o CPF ja esta cadastrado.", 409
    except SQLAlchemyError:
        return "Nao foi possivel salvar o usuario. Tente novamente.", 500

    if request.method == "POST":
        return redirect("/editar", code=303)
    return render_template("editar_usuario.html", usuario=usuario)
@app.route("/remover/<cpf>", methods=["POST"])
def remover(cpf):
    try:
        with engine.begin() as conexao:
            conexao.execute(
                text('DELETE FROM "clientes" WHERE cpf = :cpf'),
                {"cpf": cpf},
            )
    except SQLAlchemyError:
        return "Nao foi possivel remover o usuario. Tente novamente.", 500

    return redirect("/editar", code=303)


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
                text('DELETE FROM "clientes" WHERE cpf = :cpf'),
                {"cpf": session["cpf"]},
            )
    except SQLAlchemyError:
        return "Nao foi possivel cancelar o cadastro. Tente novamente.", 500
    
    session.pop("cpf", None)
    session.pop("rfid", None)
    leituraRFID = None
    return redirect("/", code=303)


if __name__ == "__main__":
    try:
        app.run(debug=True, port=5000, use_reloader=False)
    finally:
        mqtt.disconnect()
        threadMQTT.join(timeout=2)
