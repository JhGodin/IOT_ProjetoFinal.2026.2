from flask import request, redirect, render_template, session
from sqlalchemy import text

from credenciais.credenciais import ADMIN_USUARIO, ADMIN_SENHA

avisoAtual = None


def registrarNebu(app, mqtt, engine):
    def buscarTotens():
        with engine.connect() as conexao:
            linhas = conexao.execute(
                text('SELECT id, tipo FROM "totens" ORDER BY tipo, id')
            ).all()
        return [{"id": linha.id, "tipo": linha.tipo} for linha in linhas]

    def topicoAviso(idTotem):
        return f"totem/{idTotem}/aviso"

    def tratarLoginAdmin():
        if request.form.get("usuario") != ADMIN_USUARIO or request.form.get("senha") != ADMIN_SENHA:
            return redirect("/avisos", code=303)

        session["admin"] = True
        return redirect("/avisos", code=303)

    def tratarAviso():
        global avisoAtual

        if request.form.get("acao") == "limpar":
            for idTotem in avisoAtual["totens"]:
                mqtt.publish(topicoAviso(idTotem), payload="", qos=1, retain=True)
            avisoAtual = None
            return redirect("/avisos", code=303)

        titulo = request.form.get("titulo", "").strip()
        corpo = request.form.get("corpo", "").strip()
        totens = [int(id) for id in request.form.getlist("totens")]

        for idTotem in totens:
            mqtt.publish(topicoAviso(idTotem), payload=f"{titulo}\n{corpo}", qos=1, retain=True)

        avisoAtual = {"titulo": titulo, "corpo": corpo, "totens": totens}
        return redirect("/avisos", code=303)

    @app.route("/avisos", methods=["GET", "POST"])
    def avisos():
        if not session.get("admin"):
            if request.method == "POST":
                return tratarLoginAdmin()
            return render_template("admin_login.html")

        if request.method == "POST":
            return tratarAviso()

        totens = buscarTotens()
        chamados = []
        if avisoAtual:
            idsChamados = set(avisoAtual["totens"])
            chamados = [totem for totem in totens if totem["id"] in idsChamados]

        return render_template("avisos.html", aviso=avisoAtual, totens=totens, chamados=chamados)

    @app.route("/avisos/sair", methods=["POST"])
    def sairAdmin():
        session.pop("admin", None)
        return redirect("/avisos", code=303)
