# Analisador Léxico de COBOL

ACH2087 — Construção de Compiladores

## Integrantes do grupo

Nome: Vítor Benvenisti Laguna Navarenho

Número USP: 14749425

## Uso

Requisitos: compilador C11 e Make. Os testes e a geração do relatório usam Python 3.

```sh
make
./cobol-lexer arquivo.cob
make test
```

A saída contém `linha:coluna`, categoria e lexema de cada token.
Diagnósticos são enviados para stderr. Códigos de saída: 0 para sucesso,
1 para erro léxico e 2 para falha de execução.

O scanner processa COBOL em formato fixo: ignora as colunas 1–6 e 73 em diante,
interpreta o indicador da coluna 7 e analisa o código nas colunas 8–72.

O [relatório](relatorio/RELATORIO.md) descreve a implementação e apresenta os
[resultados dos testes](relatorio/RESULTADOS.md). Para atualizar as saídas:
`make report`.

O código gerado pelo Flex está incluído. Após editar `src/lexer.l`,
execute `make regen` com Flex instalado.
