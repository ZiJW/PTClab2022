#include "common.h"
#include "stree.h"
#include "type.h"
#include "semantic.h"
#include "asm.h"
#ifdef COTEST
#include <unistd.h>
#endif
extern int yyparse();
extern void yyrestart();
extern ll lines;
extern ll columns;
extern bool LEXFAULT;
extern bool BIFAULT;
extern bool SAFAULT;
extern int yylineno;
FILE * fin, * fout, * ftest;
char finname[512];
char foutname[512];
char ftestname[512];
char cmd[1024];
#ifdef STDINPUT
int main() {
    yyparse();
}
#else
int main(int argc, char **argv)
{
    #ifdef COTEST
    extern int errline;
    for (int i = 1; i < argc; i++)
    {
        *strrchr(argv[i], 'c') = '\0';
        strcpy(finname, argv[i]); strcat(finname, "cmm");
        strcpy(foutname, argv[i]); strcat(foutname, "out");
        strcpy(ftestname, argv[i]); strcat(ftestname, "output");
        fin = fopen(finname, "r");
        fout = fopen(foutname, "w");
        if (!fin || !fout)
        {
            perror(argv[i]);
            return 1;
        }
        printf("Test : %s\n", argv[i]);
        yyrestart(fin);
        yylineno = 1; errline = 0;
        lines = 1, columns = 0, LEXFAULT = false, BIFAULT = false;
        yyparse();
        if(!LEXFAULT && !BIFAULT) SemanticAnalysis();
        fclose(fin);
        fclose(fout);
        printf("done\n");
    }
    return 0;
    #endif
    #ifdef SUBMIT
    fout = stdout;
    for (int i = 1; i < argc; i++)
    {
        fin = fopen(argv[i], "r");
        if (!fin)
        {
            perror(argv[i]);
            return 1;
        }
        yyrestart(fin);
        yylineno = 1;
        yyparse();
        lines = 1, columns = 0, LEXFAULT = false, BIFAULT = false;
        if(!LEXFAULT && !BIFAULT) SemanticAnalysis();
        fclose(fin);
    }
    return 0;
    #endif
    #ifndef SUBMIT
    #ifndef COTEST
    fin = fopen(argv[1], "r");
    fout = fopen(argv[2], "w");
    yylineno = 1;
    lines = 1, columns = 0, LEXFAULT = false, BIFAULT = false;
    yyrestart(fin);
    yyparse();
    if (!LEXFAULT && !BIFAULT) SemanticAnalysis();
    if (!SAFAULT) {
        //IROutput();
        IR2ASM();
    }
    //dfsprint(SyntaxTreeRoot, 0);
    fclose(fin);
    fclose(fout);
    return 0;
    #endif
    #endif
}
#endif

