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

    consulta = text('SELECT cpf FROM "_totem_ProjetoFinal" WHERE cpf = :cpf')

    try:
        with engine.begin() as conexao:
            if conexao.execute(consulta, {"cpf": cpfUsuario}).first() is not None:
                return render_template("cpf_ja_registrado.html"), 409

            nomeUsuario = request.form.get("nome", "").strip()
            link = "http://192.168.4.1/loja?id=" + token_urlsafe(32) + cpfUsuario
            comando = text(
                'INSERT INTO "_totem_ProjetoFinal" (nome, cpf, link) '
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
            consulta = text('SELECT cpf FROM "_totem_ProjetoFinal" WHERE rfid = :rfid')
            
            if conexao.execute(consulta, {"rfid": uid}) is not None:
                leituraRFID = None
                return {"estado": "erro", "mensagem": "RFID ja cadastrado. Aproxime outro cartao."}, 409

            resultado = conexao.execute(
                text('UPDATE "_totem_ProjetoFinal" SET rfid = :rfid '
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
                text('DELETE FROM "_totem_ProjetoFinal" WHERE cpf = :cpf'),
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
