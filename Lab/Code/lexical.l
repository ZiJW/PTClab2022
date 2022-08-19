%option noyywrap
%option yylineno
%{
#include "common.h"
int yycolumn = 1;
  #define YY_USER_ACTION \
   yylloc.first_line = yylloc.last_line = yylineno; \
   yylloc.first_column = yycolumn; \
   yylloc.last_column = yycolumn + yyleng - 1; \
   yycolumn += yyleng;

#include "syntax.tab.h"
extern FILE * fout;

bool LEXFAULT = false;

ll columns = 0;
ll lines = 1;


#define Helper_lcv(x, y) \
    YYSTYPE tmp; \
    tmp = (YYSTYPE) malloc(sizeof(struct SNode)); \
    tmp->empty = false; tmp->line = lines; tmp->column = columns; tmp->x = y; tmp->relop = RELOP_NONE;
#define Helper_lc \
    YYSTYPE tmp; \
    tmp = (YYSTYPE) malloc(sizeof(struct SNode)); \
    tmp->empty = false; tmp->line = lines; tmp->column = columns; tmp->relop = RELOP_NONE;
#define Helper_lcr(x) \
    YYSTYPE tmp; \
    tmp = (YYSTYPE) malloc(sizeof(struct SNode)); \
    tmp->empty = false; tmp->line = lines; tmp->column = columns; tmp->relop = RELOP_##x;
#define Helper_lctype \
    {tmp = (YYSTYPE) malloc(sizeof(struct SNode)); tmp->empty = false; tmp->line = lines; tmp->column = columns; strcpy(tmp->idval, yytext);}
#define LexPrintInfo_lc(name) \
    printf("Tokens : \033[1;32;40m"#name"\033[0m position : line \033[1;33;40m%lld\033[0m, column \033[1;33;40m%lld\033[0m\n", tmp->line, tmp->column)
#define LexPrintInfo_lcv(name, type, val)\
    printf("Tokens : \033[1;32;40m"#name"\033[0m  value : \033[1;35;40m"#type"\033[0m  position : line \033[1;33;40m%lld\033[0m, column \033[1;33;40m%lld\033[0m\n", val, tmp->line, tmp->column);

#ifdef LEXDEBUG
#define CHelper(x) { \
    Helper_lc \
    tmp->tokennum = x; \
    columns += yyleng; \
    LexPrintInfo_lc(#x); \
    yylval = tmp; \
    return x;\
}
#else
#define CHelper(x) { \
    Helper_lc \
    tmp->tokennum = x; \
    columns += yyleng; \
    yylval = tmp; \
    return x;\
}
#endif

#ifdef LEXDEBUG
#define CHelper_r(x) { \
    Helper_lcr(x) \
    tmp->tokennum = RELOP; \
    columns += yyleng; \
    LexPrintInfo_lc(RELOP_##x); \
    yylval = tmp; \
    return RELOP;\
}
#else
#define CHelper_r(x) { \
    Helper_lcr(x) \
    tmp->tokennum = RELOP; \
    columns += yyleng; \
    yylval = tmp; \
    return RELOP;\
}
#endif
%}
digit       [0-9]
digit_no_zero [1-9]
letter      [a-zA-Z_]
hexdigit    [0-9a-fA-F]
octdigit    [0-7]
int         ({digit_no_zero}{digit}*|0)
oct         0({octdigit}+)
hex         0[xX]({hexdigit}+)
float       ([0-9]+\.[0-9]+)|(([0-9]*\.?[0-9]+|[0-9]+\.)[eE][+-]?[0-9]+)
linecomment (\/\/.*\n)
comment     (\/\*([^*]|(\*+[^\/]))*\*\/)
id          ([a-zA-Z_][0-9a-zA-Z_]*)
relop       (>|<|>=|<=|==|!=)
%%
{linecomment} {
    #ifdef LEXDEBUG
    printf("Tokens : line comment  value : no  position : line %lld, column %lld\n", lines, columns);
    #endif
    lines += 1;
    columns = 0;
}
{comment} {
    assert(yyleng > 3);
    //calculate the line and column
    #ifdef  LEXDEBUG
    printf("Tokens : \033[1;32;40mcomment\033[0m position : start line : \033[1;33;40m%lld\033[0m, start column : \033[1;33;40m%lld\033[0m ", lines, columns);
    #endif
    int pos = 0, rec = 0;
    for (;pos < yyleng; pos++) {
        if (yytext[pos] == '\n') {
            lines += 1;
            rec = pos;
        }
    }
    if (rec != 0) columns = yyleng - rec - 1;
    else columns += yyleng;
    #ifdef  LEXDEBUG
    printf("end line : \033[1;33;40m%lld\033[0m, end column : \033[1;33;40m%lld\033[0m\n", lines, columns);
    #endif
}
while   CHelper(WHILE)
else    CHelper(ELSE)
if      CHelper(IF)
return  CHelper(RETURN)
struct  CHelper(STRUCT)
\}      CHelper(RC)
\{      CHelper(LC)
\]      CHelper(RB)
\[      CHelper(LB)
\)      CHelper(RP)
\(      CHelper(LP)
(int|float) {
    YYSTYPE tmp;
    Helper_lctype
    tmp->tokennum = TYPE;
    columns += yyleng;
    #ifdef LEXDEBUG
    if (yytext[0] == 'i') LexPrintInfo_lc(TYPE_INT); else LexPrintInfo_lc(TYPE_FLOAT);
    #endif
    yylval = tmp;
    return TYPE;
}
!       CHelper(NOT)
\.      CHelper(DOT)
\|\|    CHelper(OR)
&&      CHelper(AND)
\/      CHelper(DIV)
\*      CHelper(STAR)
-       CHelper(MINUS)
\+      CHelper(PLUS)
>       CHelper_r(G)
\<      CHelper_r(L)
>=      CHelper_r(GE)
\<=     CHelper_r(LE)
==      CHelper_r(EQ)
!=      CHelper_r(NEQ)
=       CHelper(ASSIGNOP)
,       CHelper(COMMA)
;       CHelper(SEMI)
{hex} {
    char * ptr;
    Helper_lcv(ival, (unsigned int) strtol(yytext + 2, &ptr, 16))
    tmp->tokennum = INT;
    columns += yyleng;
    #ifdef LEXDEBUG
    LexPrintInfo_lcv(int(hex), %u, tmp->ival)
    #endif
    yylval = tmp;
    return INT;
}
{oct} {
    char * ptr; 
    Helper_lcv(ival, (unsigned int) strtol(yytext, &ptr, 8))
    tmp->tokennum = INT;
    columns += yyleng;
    #ifdef LEXDEBUG
    LexPrintInfo_lcv(int(oct), %u, tmp->ival)
    #endif
    yylval = tmp;
    return INT;
}
{int} {
    Helper_lcv(ival, (unsigned int) atoi(yytext))
    tmp->tokennum = INT;
    columns += yyleng;
    #ifdef LEXDEBUG
    LexPrintInfo_lcv(int(dec), %u, tmp->ival)
    #endif
    yylval = tmp;
    return INT;
}
{float} {
    Helper_lcv(fval, (float) atof(yytext))
    tmp->tokennum = FLOAT;
    columns += yyleng;
    #ifdef LEXDEBUG
    LexPrintInfo_lcv(float, %f, tmp->fval)
    #endif
    yylval = tmp;
    return FLOAT;
}
{id} {
    assert(yyleng < MAXIDLENGTH);
    Helper_lc
    tmp->tokennum = ID;
    strcpy(tmp->idval, yytext);
    #ifdef LEXDEBUG
    LexPrintInfo_lcv(ID, %s, tmp->idval)
    #endif
    yylval = tmp;
    return ID;
}
\n {
    lines += 1;
    columns = 1;
    yycolumn = 1;
    #ifdef STDINPUT
    return yytext[0];
    #endif
}
[ \f\r\t\v] {
   columns++; 
}
(.) {
    fprintf(fout, "Error type A at line %lld : Mysterious character : \"%s\"\n", lines, yytext);
    LEXFAULT = true;
    return 0;
}
%%