%option reentrant noyywrap nounput noinput nodefault never-interactive
%option prefix="cobol_yy"
%option extra-type="LexerContext *"
%x PICWAIT

%{
#include "lexer_internal.h"

#define YY_USER_ACTION lexer_note_match(yyextra, (size_t)yyleng);
/* yyless must also rewind our physical-source cursor. */
#define KEEP_PREFIX(n) do { yyextra->offset -= (size_t)yyleng - (n); yyless(n); } while (0)
%}

WORD        [A-Za-z0-9_][A-Za-z0-9_-]*
DECIMAL     [+-]?[0-9]+\.[0-9]+
INTEGER     [+-]?[0-9]+
SIGNED_BAD  [+-][A-Za-z0-9_-]+
BADDECIMAL  [+-]?[0-9]+\.[0-9]+[A-Za-z_][A-Za-z0-9_-]*
PICATOM     ([AX9SVPZB0+*/,$-]|CR|DB|\(0*[1-9][0-9]*\))
PICMASK     {PICATOM}+(\.{PICATOM}+)*
DSTRING     \"[^\"\n]*\"
SSTRING     \'[^\'\n]*\'

%%

[ \t\r\n]+             { }
{BADDECIMAL}            { return lexer_emit(yyextra, TOK_ERROR, yytext, yyleng, "malformed numeric literal"); }
{DECIMAL}|{INTEGER}     {
                            if ((yytext[0] == '+' || yytext[0] == '-') &&
                                yyextra->match_start > 0) {
                                char previous = yyextra->source->text[yyextra->match_start - 1];
                                if (previous != ' ' && previous != '\t' && previous != '\n' &&
                                    previous != '(') {
                                    KEEP_PREFIX(1);
                                    return lexer_emit(yyextra, yytext[0] == '+' ? TOK_PLUS : TOK_MINUS,
                                                      yytext, yyleng, NULL);
                                }
                            }
                            return lexer_emit(yyextra, TOK_NUMBER, yytext, yyleng, NULL);
                        }
{SIGNED_BAD}            {
                            size_t start = yyextra->match_start;
                            char previous = start == 0 ? ' ' : yyextra->source->text[start - 1];
                            if (yytext[0] == '+' ||
                                (previous != ' ' && previous != '\t' &&
                                 previous != '\n' && previous != '(')) {
                                KEEP_PREFIX(1);
                                return lexer_emit(yyextra, yytext[0] == '+' ? TOK_PLUS : TOK_MINUS,
                                                  yytext, yyleng, NULL);
                            }
                            return lexer_emit(yyextra, TOK_ERROR, yytext, yyleng,
                                              "malformed signed literal or identifier");
                        }
{WORD}                  {
                            TokenType type = lexer_emit_word(yyextra, yytext, yyleng);
                            if (type == TOK_PIC || type == TOK_PICTURE) { BEGIN(PICWAIT); }
                            return type;
                        }
<PICWAIT>[ \t\r\n]+    { /* a picture clause can span ordinary lines */ }
<PICWAIT>[Ii][Ss]       { return lexer_emit(yyextra, TOK_IS, yytext, yyleng, NULL); }
<PICWAIT>(?i:{PICMASK}) {
                            BEGIN(INITIAL);
                            return lexer_emit(yyextra, TOK_PICTURE_STRING, yytext, yyleng, NULL);
                        }
<PICWAIT>[^ \t\r\n.;]+ {
                            BEGIN(INITIAL);
                            return lexer_emit(yyextra, TOK_ERROR, yytext, yyleng, "malformed picture string");
                        }
<PICWAIT>[.;]           {
                            KEEP_PREFIX(0);
                            BEGIN(INITIAL);
                            return lexer_emit(yyextra, TOK_ERROR, "", 0, "missing picture string");
                        }
<PICWAIT><<EOF>>        {
                            BEGIN(INITIAL);
                            yyextra->match_start = yyextra->offset;
                            return lexer_emit(yyextra, TOK_ERROR, "", 0, "missing picture string");
                        }
{DSTRING}|{SSTRING}     { return lexer_emit(yyextra, TOK_STRING, yytext, yyleng, NULL); }
\"[^\"\n]*            { return lexer_emit(yyextra, TOK_ERROR, yytext, yyleng,
                                             "unterminated double-quoted string"); }
\'[^\'\n]*             { return lexer_emit(yyextra, TOK_ERROR, yytext, yyleng,
                                             "unterminated single-quoted string"); }
"."                     { return lexer_emit(yyextra, TOK_PERIOD, yytext, yyleng, NULL); }
","                     { return lexer_emit(yyextra, TOK_COMMA, yytext, yyleng, NULL); }
";"                     { return lexer_emit(yyextra, TOK_SEMICOLON, yytext, yyleng, NULL); }
"("                     { return lexer_emit(yyextra, TOK_LPAREN, yytext, yyleng, NULL); }
")"                     { return lexer_emit(yyextra, TOK_RPAREN, yytext, yyleng, NULL); }
">="                    { return lexer_emit(yyextra, TOK_GE, yytext, yyleng, NULL); }
"<="                    { return lexer_emit(yyextra, TOK_LE, yytext, yyleng, NULL); }
"<>"                    { return lexer_emit(yyextra, TOK_NE, yytext, yyleng, NULL); }
"**"                    { return lexer_emit(yyextra, TOK_POWER, yytext, yyleng, NULL); }
"="                     { return lexer_emit(yyextra, TOK_EQ, yytext, yyleng, NULL); }
">"                     { return lexer_emit(yyextra, TOK_GT, yytext, yyleng, NULL); }
"<"                     { return lexer_emit(yyextra, TOK_LT, yytext, yyleng, NULL); }
"+"                     { return lexer_emit(yyextra, TOK_PLUS, yytext, yyleng, NULL); }
"-"                     { return lexer_emit(yyextra, TOK_MINUS, yytext, yyleng, NULL); }
"*"                     { return lexer_emit(yyextra, TOK_STAR, yytext, yyleng, NULL); }
"/"                     { return lexer_emit(yyextra, TOK_SLASH, yytext, yyleng, NULL); }
\x1f                    { return lexer_emit(yyextra, TOK_ERROR, "-", 1,
                                             "continuation indicator without a preceding unfinished source line"); }
.                       { return lexer_emit(yyextra, TOK_ERROR, yytext, yyleng,
                                             "invalid character"); }

%%
