# Relatório — Analisador Léxico de COBOL

ACH2087 — Construção de Compiladores

Vítor Benvenisti Laguna Navarenho — Nº USP 14749425

## Implementação

O analisador utiliza C11 e Flex. Reconhece palavras reservadas, identificadores,
números com sinal opcional, strings, operadores, pontuação e máscaras PIC.
Cada token preserva o lexema e sua linha e coluna física.

- `fixed_format.c`: tratamento das colunas, comentários e continuação de linhas;
- `lexer.l`: regras de reconhecimento;
- `lexer.c` e `keywords.def`: interface do lexer, palavras reservadas e validação
  dos identificadores;
- `token.c` e `main.c`: gerenciamento dos tokens e interface de linha de comando.

Erros léxicos produzem diagnósticos e permitem continuar a análise.
A função `lexer_next` retorna um token por chamada até `TOK_EOF`.

## Testes

Foram utilizados os sete programas de
[ebasso/cobol-exemplos](https://github.com/ebasso/cobol-exemplos/tree/dbe15b74769c994ceda82b48b2640fb208907d3c),
preservados em `tests/examples/` com a licença original.

Os sete programas foram analisados sem erros léxicos.
As contagens e as saídas completas estão em [RESULTADOS.md](RESULTADOS.md).
`make report` reproduz essas saídas.

`make test` também verifica categorias, lexemas, posições, limites de
identificadores, operadores, máscaras PIC, continuação, recuperação de erros
e chamadas à API até o fim do arquivo.

## Regras adotadas

- Palavras reservadas não diferenciam maiúsculas de minúsculas. A lista está
  em `src/keywords.def`.
- Identificadores aceitam letras, dígitos, `_` e hífens internos, com até
  30 caracteres.
- Linhas de comentário e de depuração (`D`/`d` na coluna 7) são ignoradas.
- A continuação une palavras que começam na coluna 8; espaços iniciais
  separam palavras completas. Em strings abertas, aceita-se a repetição do
  delimitador. Espaços finais das linhas são descartados.
- Sinais após espaço ou parêntese de abertura podem integrar números.
  `1+2` é separado em número, operador e número. `3-4` é um identificador;
  a subtração deve ser escrita como `3 - 4`.
- Após `PIC`/`PICTURE`, com `IS` opcional, a máscara recebe a categoria
  `PICTURE_STRING`; o ponto final da sentença é separado. Não há validação
  semântica da máscara.
- `KW_ERROR` e `KW_STRING` identificam as palavras reservadas `ERROR` e
  `STRING`, distinguindo-as do token de erro e do literal string.
