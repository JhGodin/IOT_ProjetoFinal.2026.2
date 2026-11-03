-- ///////gabriel/////// esquema do banco do totem (PostgreSQL)
-- Rodar uma vez no Beekeeper Studio, no banco do grupo.
-- Precos em centavos (inteiro), como na tabela produtos do Projeto 04: 2500 = R$ 25.00


-- Usuarios: tabela que ja existe e e usada pelo cadastro (rotas / e /rfid do sites.py).
-- Fica aqui so como documentacao das colunas; nao precisa rodar de novo.
-- "_totem_ProjetoFinal" (nome, cpf, link, rfid)


-- Produtos do cardapio. O codigo de cada produto vira o nome do campo no formulario:
-- codigo 1 -> campo "item1", codigo 2 -> "item2" ... (mesmo padrao do cardapio.html)
CREATE TABLE "_totem_produtos" (
    codigo INTEGER PRIMARY KEY,
    nome TEXT NOT NULL,
    preco INTEGER NOT NULL
);

INSERT INTO "_totem_produtos" (codigo, nome, preco) VALUES
(1, 'Hambúrguer', 2500),
(2, 'Batata frita', 1500),
(3, 'Refrigerante', 800),
(4, 'Água', 500);


-- Consumos: uma linha por produto de cada pedido feito pelo site (/loja).
-- id e data_hora sao automaticos (como a tabela vendas do P04 e consumos do P05).
-- preco guarda o preco do produto no momento da compra.
CREATE TABLE "_totem_consumos" (
    id SERIAL PRIMARY KEY,
    data_hora TIMESTAMP DEFAULT NOW(),
    cpf TEXT NOT NULL,
    codigo_produto INTEGER NOT NULL,
    quantidade INTEGER NOT NULL,
    preco INTEGER NOT NULL
);


-- Consultas uteis -------------------------------------------------------------

-- Total a pagar de um usuario pelo CPF (e o que aparece na tela de confirmacao)
SELECT SUM(quantidade * preco) AS total
FROM "_totem_consumos"
WHERE cpf = '12345678909';

-- Total a pagar pelo RFID (para o pagamento na saida - Lucas)
SELECT SUM(c.quantidade * c.preco) AS total
FROM "_totem_consumos" c
JOIN "_totem_ProjetoFinal" u ON u.cpf = c.cpf
WHERE u.rfid = 'E0 E0 95 13';

-- Quantidade vendida por produto (base do dashboard da semana 3)
SELECT p.nome, SUM(c.quantidade) AS vendidos
FROM "_totem_consumos" c
JOIN "_totem_produtos" p ON p.codigo = c.codigo_produto
GROUP BY p.nome
ORDER BY vendidos DESC;
