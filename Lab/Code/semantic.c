#include "semantic.h"
#include "ir.h"

bool SAFAULT;

extern FILE* fout;

typedef struct SAErrorMsg {
    char * msg1;
    char * msg2;
} SAErrorMsg;

const SAErrorMsg SAETable[] = {
    {" Default error", ""},
    {" Undefined variable", ""},
    {" Undefined function", ""},
    {" Redefined variable", ""},
    {" Redefined function", ""},
    {" Type mismatched for assignment", ""},
    {" The left-hand side of an assignment must be a variable", ""},
    {" Type mismatched for operands", ""},
    {" Type mismatched for return", ""},
    {" Function", " is not applicable for arguments"},
    {"", " is not an array"},
    {"", " is not a function"},
    {"", " is not an integer"},
    {" Illegal use of", ""},
    {" Non-existent field", ""},
    {" Redefined field", ""},
    {" Duplicated name", ""},
    {" Undefined structure", ""},
    {" Undefined function", ""},
    {" Inconsistent declaration of function", ""}
};

void SemanticAnalysis() {
    SAFAULT = false;
    IRinit();
    SATypeInit();
    STPrepare();
    STStackPrepare();
    ReadWriteInit();
    SAHandleProgram(SyntaxTreeRoot);
    SemanticCheck();
    STDestoryall();
}

void SemanticCheck() {
    funcSTTranverse(CheckFunctionDefined);
}

void CheckFunctionDefined(void *x) {
    STElement * tmp = (STElement*) x;
    SAType * type = tmp->type;
    assert(type->kind == FUNCTION);
    if (type->function.defined == false) ThrowError(SAError_Undefined_Decleared_Function, type->function.line, tmp->name, NULL);
}

void ThrowError(enum SAErrorType num, int line, char *msg1, char *msg2) {
    SAFAULT = true;
    fprintf(fout, "Error type %d at Line %d:", num, line);
    fprintf(fout, "%s", SAETable[num].msg1);
    if (msg1) fprintf(fout, " \"%s\"", msg1);
    fprintf(fout, "%s", SAETable[num].msg2);
    if (msg2) fprintf(fout, " \"%s\"", msg2);
    fprintf(fout, ".\n");
}