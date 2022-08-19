#ifndef TYPE_H
#define TYPE_H
#include "common.h"
enum SABasicType
{
    VOID,
    BASIC,
    ARRAY,
    STRUCTURE,
    FUNCTION
};

struct SAFieldList;
typedef struct SAFieldList SAFieldList;

typedef struct SAType
{
    enum SABasicType kind;
    size_t size;
    struct SAType * parent;
    union
    {
        int basic;
        struct
        {
            int size;
            enum SABasicType kind;
            struct SAType *type;
        } array;
        SAFieldList *structure;
        struct
        {
            bool defined;
            int line;
            struct SAType *returntype;
            struct SAFieldList *parameter;

        } function;
    };
} SAType;

struct SAFieldList
{
    char *name;
    struct SAType *type;
    struct SAFieldList *next;
};

void SATypeInit();
void ReadWriteInit();
void SAHandleProgram(struct SNode * pro);
void SAHandleExtDefList(struct SNode * extdeflist);
void SAHandleExtDef(struct SNode *extdef);
void SAHandleExtDecList(struct SNode * extdec, SAType * type);

SAType * SAHandleSpecifier(struct SNode * spe);
SAType * SAHandleStructSpecifier(struct SNode * stru);

SAFieldList *SAHandleVarDec(struct SNode *vardec, SAType *type, bool inST, bool return_or_not, bool stru, bool func);
bool SAHandleFunDec(struct SNode *funcdec, SAType *type, bool define);
SAFieldList * SAHandleVarList(struct SNode *varlist, bool define);
SAFieldList * SAHandleParamDec(struct SNode *paramdec, bool define);

void SAHandleCompSt(struct SNode * cmpst, bool newstack);
void SAHandleStmtList(struct SNode * stmtlist);
void SAHandleStmt(struct SNode * stmt);

SAFieldList * SAHandleDefList(struct SNode * deflist, bool stru);
SAFieldList * SAHandleDef(struct SNode *def, bool stru);
SAFieldList * SAHandleDecList(struct SNode *declist, SAType * type, bool stru);
SAFieldList * SAHandleDec(struct SNode *dec, SAType * type, bool stru);

SAType * SAHandleExp(struct SNode * exp);
SAFieldList * SAHandleArgs(struct SNode *args);

bool SACompareType(SAType * a, SAType * b);
bool SACompareField(SAFieldList * a, SAFieldList * b);
SAType * GetNewSAType();
SAType * GetParentSAType(SAType * tmp);
SAType * FunctionReturnType;
void showField(SAFieldList *a);
void show();
#endif