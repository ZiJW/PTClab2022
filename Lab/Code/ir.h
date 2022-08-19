#ifndef IR_H
#define IR_H
#include "common.h"
typedef struct SNode SNode;
#include "type.h"
#include "rbtree.h"

enum IROperandType {
    IR_OP_NONE,
    IR_OP_TEMP,
    IR_OP_LABEL,
    IR_OP_VARIABLE,
    IR_OP_ADDRESS,
    IR_OP_INSTANT_INT,
    IR_OP_INSTANT_FLOAT,
    IR_OP_FUNCTION,
    IR_OP_RELOP
};

enum IRCodeType {
    IR_CODE_LABEL,
    IR_CODE_FUNCTION,
    IR_CODE_ASSIGN,
    IR_CODE_ADD,
    IR_CODE_SUB,
    IR_CODE_MUL,
    IR_CODE_DIV,
    IR_CODE_GETADDR,
    IR_CODE_LOAD,
    IR_CODE_SAVE,
    IR_CODE_GOTO,
    IR_CODE_GOTO_RELOP,
    IR_CODE_RETURN,
    IR_CODE_DEC,
    IR_CODE_ARG,
    IR_CODE_CALL,
    IR_CODE_PARAM,
    IR_CODE_READ,
    IR_CODE_WRITE
};

typedef struct IROperand {
    enum IROperandType kind;
    unsigned int size;
    int offset;
    union {
        unsigned int num;
        int ivalue;
        float fvalue;
        const char *name;
        enum RELOP_STATE relop;
    };
} IROperand;

typedef struct IRCode {
    enum IRCodeType kind;
    union {
        struct {
            struct IROperand label;
        } label;
        struct {
            struct IROperand function;
            struct RBNode * root;
        } function;
        struct {
            struct IROperand left, right;
        } assign, getaddr, load, save;
        struct {
            struct IROperand op1, op2, result;
        } binop;
        struct {
            struct IROperand destination;
        } jmp;
        struct {
            struct IROperand op1, relop, op2, destination;
        } jmp_relop;
        struct {
            struct IROperand retval;
        } ret;
        struct {
            struct IROperand variable, size;
        } dec;
        struct {
            struct IROperand result, function;
        } call;
        struct {
            struct IROperand variable;
        } arg, param, read, write;
    };
    struct IRCode * prev, * next;
} IRCode;

typedef struct IRCodeList {
    struct IRCode * head, * end;
} IRCodeList;

typedef struct IRCodePair {
    struct IRCodeList list;
    struct IROperand op;
    SAType * type;
} IRCodePair;

void IRinit();

IRCodePair IRTranslateExp(SNode * exp, IROperand place, bool deref);
IRCodeList IRTranslateStmt(SNode * stmt);
IRCodeList IRTranslatePreCond(SNode *exp, IROperand op);
IRCodeList IRTranslateCond(SNode * exp, IROperand labeltrue, IROperand labelfalse);
IRCodeList IRTranslateArgs(SNode * args, IRCodeList * arglist, SAFieldList * field);
IRCodeList IRTranslateCompSt(SNode * compst);
IRCodeList IRTranslateDefList(SNode * deflist);
IRCodeList IRTranslateStmtList(SNode * stmtlist);
IRCodeList IRTranslateDef(SNode * def);
IRCodeList IRTranslateDecList(SNode * declist);
IRCodeList IRTranslateDec(SNode *dec);
IRCodeList IRTranslateVarDec(SNode * VarDec);
IRCodeList IRTranslateStmt(SNode * stmt);

void IRSAConnectionFunDec(SNode * fundec);
void IRSAConnectionCompSt(SNode * compst);
void IRSAConnectionDefList(SNode *deflist);
void IRSAConnectionStmtList(SNode *stmtlist);
void IRSAGenerateCode(SNode *fundec, SNode * compst);

IROperand IRNewOperandNone();
IROperand IRNewOperandTemp();
IROperand IRNewOperandLabel();
IROperand IRNewOperandVariable(char * name, bool dec);
IROperand IRNewOperandInstantINT(int tmp);
IROperand IRNewOperandInstantFLOAT(float tmp);
IROperand IRNewOperandFunction(char * name);
IROperand IRNewOperandRelop(enum RELOP_STATE relop);

IRCode * IRNewCode(enum IRCodeType kind);
IRCodeList IRAppendCode(IRCodeList list, IRCode *code);
IRCodeList IRMergeCodeList(IRCodeList lista, IRCodeList listb);
IRCodeList IRRemoveCode(IRCodeList list, IRCode *code);
IRCodePair IRWrapCode(IRCodeList list, IROperand op, SAType * type);
void IRDestoryCode(IRCodeList list);
void IRFreshCode(IRCodeList list);

size_t IROutputOperand(char *s, IROperand * op);
size_t IROutputCode(char *s, IRCode *code);
void IROutputCodeList(IRCodeList *list);
void IROutput();

#endif