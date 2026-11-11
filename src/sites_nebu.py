from flask import request, redirect, render_template, session

# ///////nebu/////// credenciais do organizador, para a pagina /avisos
from credenciais.credenciais import ADMIN_USUARIO, ADMIN_SENHA

# ///////nebu/////// ultimo aviso publicado, para mostrar na pagina /avisos
avisoAtual = None


def registrarNebu(app, mqtt):
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
