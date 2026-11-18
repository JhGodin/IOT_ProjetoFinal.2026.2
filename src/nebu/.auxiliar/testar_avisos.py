# Harness local só para testar /avisos (sites_nebu.py + templates/avisos.html)
# isolado do resto de sites.py -- nao precisa de Postgres nem de um broker MQTT real.
# Uso, de qualquer diretorio, com o venv do projeto:
#   /home/nebu/Desktop/IoT/.venv/bin/python src/nebu/.auxiliar/testar_avisos.py
# Depois abra http://127.0.0.1:5000/avisos (login admin/admin, da credenciais.py de teste).

import os
import sys
from secrets import token_urlsafe
from flask import Flask
from sqlalchemy import create_engine, text
from sqlalchemy.pool import StaticPool

SRC_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, SRC_DIR)

from sites_nebu import registrarNebu


class MqttFalso:
    def publish(self, topico, payload="", qos=0, retain=False):
        print(f"[mqtt fake] publish {topico!r} retain={retain}: {payload!r}", flush=True)


# Banco em memoria, só com a tabela totens -- o resto de /avisos nao toca no banco.
engine = create_engine("sqlite://", connect_args={"check_same_thread": False}, poolclass=StaticPool)
with engine.begin() as conexao:
    conexao.execute(text('CREATE TABLE totens (id INTEGER PRIMARY KEY, tipo TEXT NOT NULL)'))
    for tipo in ("Entrada", "Saida", "Comida", "Palco"):
        conexao.execute(text('INSERT INTO totens (tipo) VALUES (:tipo)'), {"tipo": tipo})

app = Flask(__name__, template_folder=os.path.join(SRC_DIR, "templates"))
app.secret_key = token_urlsafe(32)
registrarNebu(app, MqttFalso(), engine)

if __name__ == "__main__":
    app.run(debug=True, port=5000, use_reloader=False)
