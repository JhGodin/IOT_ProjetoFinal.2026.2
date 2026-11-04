# Copie para credenciais.py e preencha SOMENTE na sua maquina.
# credenciais.py esta no .gitignore e nao deve ser enviado ao GitHub.

# Tipo do banco e biblioteca de conexao:
# PostgreSQL: "postgresql+psycopg" (instalar SQLAlchemy e psycopg).
# MySQL/MariaDB: "mysql+pymysql" (instalar SQLAlchemy e pymysql).

BD_TIPO = ""  
BD_HOST = "ENDERECO_DO_SERVIDOR"  # Endereco do computador que hospeda o banco.
BD_PORTA = None  
BD_NOME = "NOME_DO_BANCO"  
BD_USUARIO = "USUARIO_DO_BANCO"  
BD_SENHA = "SENHA_DO_BANCO"  

# MQTT: ID exclusivo para o Python, diferente do ID usado pelo ESP.
MQTT_CLIENTE_ID = "ID_UNICO_DO_SERVIDOR_PYTHON"
MQTT_USUARIO = "USUARIO_MQTT"
MQTT_SENHA = "SENHA_MQTT"

# Paginas do organizador (/avisos e /produtos): usuario e senha de acesso.
ADMIN_USUARIO = "USUARIO_DO_ORGANIZADOR"
ADMIN_SENHA = "SENHA_DO_ORGANIZADOR"
