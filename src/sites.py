from flask import Flask, request, jsonify
app = Flask(__name__)


def tratarSinal():
    global nomeUsuario, cpfUsuario

    
    nomeUsuario = request.form.get("nome")
    cpfUsuario = request.form.get("cpf")

    dados = {"usuario": nomeUsuario,"cpf": cpfUsuario}
    textoJson = json.dumps(dados)

    mqtt.publish("totem/login", textoJson, qos=1, retain=False)

    # E se o CPF já estiver cadastrado?

    return redirect("/login_realizado", code=303)


@app.route("/")
def inicio():
    return "Bem Vindo!!!\nAcesse /login para continuar."

@app.route("/login", methods=["GET"])
def pagina1():
    return render_template("login.html")

@app.route("/login", methods=["POST"])
def receberLogin():
    return tratarDados()

@app.route("/login_realizado", methods=["POST"])
def receberLogin():
    return "Agradecemos ao seu Login!!!\nRegistro realizado no Banco de dados."


if __name__ == "__main__":
    app.run(debug=True, port=5000)