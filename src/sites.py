from secrets import token_urlsafe

from flask import Flask, request, redirect, render_template
from sqlalchemy import URL, create_engine, text
from sqlalchemy.exc import IntegrityError, SQLAlchemyError
from credenciais import credenciais

BD = URL.create(
    drivername=credenciais.BD_TIPO, host=credenciais.BD_HOST, port=credenciais.BD_PORTA,
    database=credenciais.BD_NOME, username=credenciais.BD_USUARIO, password=credenciais.BD_SENHA,
) if credenciais.BD_TIPO else None


engine = create_engine(BD) if BD else None
# Estabelece a conexao na inicializacao, antes de receber os cadastros.
# O engine mantem as conexoes disponiveis para reutilizacao.
if engine is not None:
    with engine.connect():
        pass

app = Flask(__name__)


def tratarDados():
    cpfUsuario = request.form.get("cpf", "").strip()

    if len(cpfUsuario) != 11 or not cpfUsuario.isascii() or not cpfUsuario.isdigit():
        return "Informe o nome e um CPF com 11 numeros.", 400

    if engine is None:
        return "ERRO: Banco de dados não configurado.", 500

    consulta = text('SELECT cpf FROM "_totem_ProjetoFinal" WHERE cpf = :cpf')
    try:
        with engine.begin() as conexao:
            # Se o CPF existir, nao gera link nem cria outro cadastro.
            if conexao.execute(consulta, {"cpf": cpfUsuario}).first() is not None:
                return redirect("/cpf_ja_registrado", code=303)

            # So continua o cadastro se o CPF ainda nao estiver no banco.
            nomeUsuario = request.form.get("nome", "").strip()
            if not nomeUsuario:
                return "Informe o nome e um CPF com 11 numeros.", 400

            # O identificador aleatorio diferencia o link sem expor o CPF.
            link = "http://192.168.4.1/loja?id=" + token_urlsafe(32)
            comando = text(
                'INSERT INTO "_totem_ProjetoFinal" (nome, cpf, link) '
                "VALUES (:nome, :cpf, :link)"
            )
            conexao.execute(comando, {"nome": nomeUsuario, "cpf": cpfUsuario, "link": link})
    except IntegrityError:
        # UNIQUE em cpf impede duplicatas se dois cadastros chegarem juntos.
        try:
            with engine.connect() as conexao:
                if conexao.execute(consulta, {"cpf": cpfUsuario}).first() is not None:
                    return redirect("/cpf_ja_registrado", code=303)
        except SQLAlchemyError:
            pass
        return "Nao foi possivel gravar o cadastro no banco de dados.", 500
    except SQLAlchemyError:
        return "Nao foi possivel gravar o cadastro no banco de dados.", 500

    # So abre o cardapio depois que a transacao confirmou a gravacao.
    return redirect(link, code=303)


@app.route("/")
def inicio():
    return "Bem Vindo!!!\nAcesse /login para continuar."

@app.route("/login", methods=["GET"])
def pagina1():
    return render_template("login.html")

@app.route("/login", methods=["POST"])
def receberLogin():
    return tratarDados()

@app.route("/login_realizado", methods=["GET"])
def loginRealizado():
    return "Dados enviados. A confirmação do cadastro ainda não foi implementada."


@app.route("/cpf_ja_registrado", methods=["GET"])
def cpfJaRegistrado():
    return render_template("cpf_ja_registrado.html")


if __name__ == "__main__":
    app.run(debug=True, port=5000)
