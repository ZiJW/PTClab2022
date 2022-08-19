%locations
%{
    #include "common.h"
    #include "stree.h"
    #include "lex.yy.c"
    void yyerror(const char* msg);
    int yylex();
    bool BIFAULT = false;
    extern FILE * fout;
    int errline = 0;

    #ifndef YYTOKEN_TABLE
        # define YYTOKEN_TABLE 1
    #endif

    #define nodet2s(x) \
        { \
        assert(x->tokennum > 0); \
        x->symbolnum = YYTRANSLATE(x->tokennum);\
        x->name = yytname[x->symbolnum];\
        }
    #define NonEmptyNode \
        (yyval) = initnode(yytname[yyr1[yyn]], false, yyr1[yyn], (yyvsp[(1) - (yyr2[yyn])])->line, (yyvsp[(1) - (yyr2[yyn])])->column);
    #define EmptyNode \
        (yyval) = initnode(yytname[yyr1[yyn]], true, yyr1[yyn], 0, 0);
    #define AddChild \
        for (int i = 1; i <= yyr2[yyn]; i++) { \
            YYSTYPE tmp = (yyvsp[(i) - (yyr2[yyn])]); \
            if (tmp->tokennum > 0) nodet2s(tmp) \
            addchild(yyval, tmp); \
        }
#ifdef BIDEBUG
    #define DEFAULT \
        printf("rule %d\n", yyrline[yyn]); \
        NonEmptyNode \
        AddChild
    #define DEFAULT_ERROR(msg, i) \
        BIFAULT = true; \
        printf("rule : %d Error type B at Line %d: %s near %s\n", yyrline[yyn], yylsp[(i) - (yyr2[yyn])].first_line, msg, yytext);
#else
    #define DEFAULT \
        NonEmptyNode \
        AddChild 
    #define DEFAULT_ERROR(msg, i) \
        BIFAULT = true; \
        if (yylsp[(i) - (yyr2[yyn])].first_line != errline) fprintf(fout, "Error type B at Line %d: %s\n", yylsp[(i) - (yyr2[yyn])].first_line, msg); \
        errline = yylsp[(i) - (yyr2[yyn])].first_line;
#endif
    #define DEFAULT_EMPTY \
        EmptyNode
%}
%token TYPE ID
%token INT
%token FLOAT
%token SEMI COMMA
%token LC RC
%token STRUCT RETURN IF WHILE

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%right ASSIGNOP
%left  OR
%left  AND
%left  RELOP
%left  PLUS MINUS
%left  STAR DIV
%right NOT NEG
%left  DOT LB RB LP RP

%%
Program :   ExtDefList              {DEFAULT; if (!BIFAULT && !LEXFAULT) SyntaxTreeRoot = yyval; }
        ;
ExtDefList  :   ExtDef ExtDefList   {DEFAULT}
            |   /* empty */         {DEFAULT_EMPTY}
            ;
ExtDef  :   Specifier ExtDecList SEMI   {DEFAULT}
        |   Specifier SEMI              {DEFAULT}
        |   Specifier FunDec CompSt     {DEFAULT}
        |   Specifier FunDec SEMI       {DEFAULT}
        |   error SEMI                  {DEFAULT_ERROR("Syntax error", 1);}
        |   error FunDec CompSt         {DEFAULT_ERROR("Syntax error", 1);}
        ;
ExtDecList  :   VarDec                  {DEFAULT}
            |   VarDec COMMA ExtDecList {DEFAULT}
            ;
Specifier   :   TYPE            {DEFAULT}
            |   StructSpecifier {DEFAULT}
            ;
StructSpecifier :   STRUCT OptTag LC DefList RC {DEFAULT}
                |   STRUCT OptTag LC error RC   {DEFAULT_ERROR("Syntax error", 4)}
                |   STRUCT OptTag error DefList RC   {DEFAULT_ERROR("Syntax error", 3)}
                |   STRUCT Tag                  {DEFAULT}
                ;
OptTag  :   ID                  {DEFAULT}
        |   /* empty */         {DEFAULT_EMPTY}
        ;
Tag     :   ID                  {DEFAULT}
        ;
VarDec  :   ID                  {DEFAULT}
        |   VarDec LB INT RB    {DEFAULT}
        |   VarDec error INT RB    {DEFAULT_ERROR("Syntax error", 2)}
        ;
FunDec  :   ID LP VarList RP    {DEFAULT}
        |   ID LP RP            {DEFAULT}
        |   error RP            {DEFAULT_ERROR("Syntax error", 1);}
        ;
VarList :   ParamDec COMMA VarList  {DEFAULT}
        |   ParamDec                {DEFAULT}
        ;
ParamDec:   Specifier VarDec    {DEFAULT}
        ;
CompSt  :   LC DefList StmtList RC  {DEFAULT}
        |   LC DefList error RC     {DEFAULT_ERROR("Syntax error", 3);}
        |   error DefList StmtList RC     {DEFAULT_ERROR("Syntax error", 1);}     
        ;
StmtList:   Stmt StmtList       {DEFAULT}
        |   /* empty */         {DEFAULT_EMPTY}
        ;
Stmt    :   Exp SEMI            {DEFAULT}
        |   CompSt              {DEFAULT}
        |   RETURN Exp SEMI     {DEFAULT}
        |   IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   {DEFAULT}
        |   IF LP Exp RP Stmt ELSE Stmt {DEFAULT}
        |   WHILE LP Exp RP Stmt{DEFAULT}
        |   WHILE error Exp RP Stmt {DEFAULT_ERROR("Missing (", 2)}
        |   WHILE LP Exp error Stmt {DEFAULT_ERROR("Missing )", 4)}
        |   error SEMI          {DEFAULT_ERROR("Syntax error", 1);}
        |   IF LP error RP Stmt %prec LOWER_THAN_ELSE {DEFAULT_ERROR("Wrong Exp between ()", 3);}
        |   IF LP error RP Stmt ELSE Stmt       {DEFAULT_ERROR("Wrong Exp between ()", 3);}
        |   IF LP Exp RP error ELSE Stmt        {DEFAULT_ERROR("Sytax error", 5);}
        |   WHILE LP error RP Stmt              {DEFAULT_ERROR("Syntax error", 3);}
        ;
DefList :   Def DefList         {DEFAULT}
        |   /* empty */         {DEFAULT_EMPTY}
        ;
Def     :   Specifier DecList SEMI  {DEFAULT}
        |   Specifier error SEMI    {DEFAULT_ERROR("Syntax error", 2);}
        |   error SEMI              {DEFAULT_ERROR("Syntax error", 1);}
        ;       
DecList :   Dec                 {DEFAULT}
        |   Dec COMMA DecList   {DEFAULT}
        ;
Dec     :   VarDec              {DEFAULT}
        |   VarDec ASSIGNOP Exp {DEFAULT}
        |   error ASSIGNOP Exp  {DEFAULT_ERROR("Syntax error", 1);}
        ;
Exp :   Exp ASSIGNOP Exp        {DEFAULT}
    |   Exp AND Exp             {DEFAULT}
    |   Exp OR Exp              {DEFAULT}
    |   Exp RELOP Exp           {DEFAULT}
    |   Exp PLUS Exp            {DEFAULT}
    |   Exp MINUS Exp           {DEFAULT}
    |   Exp STAR Exp            {DEFAULT}
    |   Exp DIV Exp             {DEFAULT}
    |   LP  Exp RP              {DEFAULT}
    |   LP error RP             {DEFAULT_ERROR("Wrong expression between ( )", 2);}
    |   LP error SEMI           {DEFAULT_ERROR("Missing \")\"", 2);}
    |   LP error RC             {DEFAULT_ERROR("Missing \";\"", 2);}
    |   MINUS Exp %prec NEG     {DEFAULT}
    |   NOT Exp                 {DEFAULT}
    |   ID LP Args RP           {DEFAULT}
    |   ID LP RP                {DEFAULT}
    |   ID LP error RP          {DEFAULT_ERROR("Wrong expression between ( ) ", 3);}
    |   ID LP error SEMI        {DEFAULT_ERROR("Missing \")\"", 3);}
    |   ID LP error RC          {DEFAULT_ERROR("Missing \";\"", 3);}
    |   Exp LB Exp RB           {DEFAULT}
    |   Exp LB error RB         {DEFAULT_ERROR("Wrong expression between [ ]", 3);}
    |   Exp LB error SEMI       {DEFAULT_ERROR("Missing \"]\"", 3);}
    |   Exp LB error RC         {DEFAULT_ERROR("Missing \";\"", 3);}
    |   Exp DOT ID              {DEFAULT}
    |   ID                      {DEFAULT}
    |   INT                     {DEFAULT}
    |   FLOAT                   {DEFAULT}
    ;
Args:  Exp COMMA Args          {DEFAULT}
    |  Exp                     {DEFAULT}
    ;
%%
void yyerror(const char *msg) {
    BIFAULT = true;
    if (LEXFAULT) return;
    if (errline == yylineno) return; // one error per line
    else errline = yylineno;
    fprintf(fout, "Error type B at Line %d: %s near '%s'.\n", yylineno, msg, yytext);
}