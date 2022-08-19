#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H
#include "rbtree.h"
#include "type.h"

typedef struct STElement {
    char * name;
    unsigned int num;
    bool ref;
    SAType * type;
} STElement;

typedef struct SymTable {
    struct RBNode * root;
    struct SymTable * prev;
} SymTable;

typedef struct SymTabStack {
    struct SymTable * start;
    struct SymTable * end;
} SymTabStack;


void STStackPrepare();
void STStackDestory();
bool STStackPush(SymTable * element);
bool STStackNewST();
bool STStackPop();

void STPrepare();
void STDestory(SymTable ** tmp);
void STDestoryall();
void STInsert(SymTable * st, char * name, SAType * type);
void funcSTInsert(char *name, SAType *type);
void struSTInsert(char *name, SAType *type);
void currSTInsert(char *name, SAType *type);

STElement * STFind(SymTable * st, char * name);
STElement * funcSTFind (char * name);
STElement * struSTFind(char *name);
STElement * currSTFind(char *name, bool thorough);
STElement * STFindAll(char * name, bool thorough);

int STECompare(const void * p1 , const void * p2);
void STTranverse(SymTable *st, void (*handle)(void *));
void funcSTTranverse(void (*handle)(void *));
#endif