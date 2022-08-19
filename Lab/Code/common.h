#ifndef COMMON
#define COMMON

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define MAXIDLENGTH 128
typedef long long ll;


enum RELOP_STATE
{
    RELOP_NONE = 0,
    RELOP_G,
    RELOP_L,
    RELOP_GE,
    RELOP_LE,
    RELOP_EQ,
    RELOP_NEQ,
    TYPE_INT,
    TYPE_FLOAT
};

struct SNode {
    const char * name;
    bool empty;
    ll line, column;
    int tokennum, symbolnum;
    enum RELOP_STATE relop;
    union
    {
        unsigned int ival;
        float fval;
        char idval[MAXIDLENGTH];
    };
    struct SNode * child, * next;
    void * codeliststart, * codelistend;
};

#define YYSTYPE struct SNode *


//#define LEXDEBUG
//#define STDINPUT
//#define BIDEBUG
//#define SADEBUG
//#define IRDEBUG
//#define ASMDEBUG
#define YYDEBUG 1
#ifdef BIDEBUG
#define YYDEBUG 1
#endif
//#define SUBMIT
//#define COTEST


struct SNode * SyntaxTreeRoot;
#endif