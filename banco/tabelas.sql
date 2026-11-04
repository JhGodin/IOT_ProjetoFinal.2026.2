-- ///////gabriel/////// consultas do banco do totem (PostgreSQL)
-- Este arquivo NAO cria tabelas: elas ja existem no banco do grupo (criadas no Beekeeper Studio).
-- Colunas que o src/sites.py espera encontrar:
--
--   clientes (id, nome, cpf, rfid, link, email, celular)
--       cadastro (rota /) e registro do cartao (rota /rfid)
--   produtos (codigo, nome, preco)
--       cardapio. preco em centavos, como no Projeto 04: 2500 = R$ 25,00
--       os produtos sao cadastrados pela pagina /produtos do servidor, sem INSERT manual
--   consumos (cpf, codigo_produto, quantidade, preco, ...)
--       uma linha por produto de cada pedido feito em /loja (como a tabela consumos do Projeto 05)
--       preco guarda o preco do produto no momento da compra
--       id e data_hora devem ser automaticos, porque o servidor nao envia esses campos
--
-- Ao ligar, o servidor confere essas colunas e mostra no terminal "Tabela ...: ok" ou o erro.


-- Corrigir links de clientes cadastrados antes (apontavam para o IP do ESP, 192.168.4.1).
-- Troque o endereco pelo do computador que roda o sites.py:
UPDATE clientes
SET link = REPLACE(link, 'http://192.168.4.1/', 'http://127.0.0.1:5000/')
WHERE link LIKE 'http://192.168.4.1/%';


-- Consultas uteis -------------------------------------------------------------

-- Link do cardapio de cada cliente
SELECT nome, link
FROM clientes;

-- Total a pagar de um cliente pelo CPF (e o que aparece na tela de confirmacao)
SELECT SUM(quantidade * preco) AS total
FROM consumos
WHERE cpf = '12345678909';

-- Total a pagar pelo RFID (para o pagamento na saida - Lucas)
SELECT SUM(c.quantidade * c.preco) AS total
FROM consumos c
JOIN clientes u ON u.cpf = c.cpf
WHERE u.rfid = 'E0 E0 95 13';

-- Quantidade vendida por produto (base do dashboard da semana 3)
SELECT p.nome, SUM(c.quantidade) AS vendidos
FROM consumos c
JOIN produtos p ON p.codigo = c.codigo_produto
GROUP BY p.nome
ORDER BY vendidos DESC;
