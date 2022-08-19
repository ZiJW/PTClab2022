#include "symtable.h"
SymTable * struTable, * funcTable, * currTable;
SymTabStack * currStack;
void STStackPrepare() {
    assert(currTable != NULL);
    currStack = (SymTabStack *) malloc(sizeof(SymTabStack));
    currStack->start = currStack->end = currTable;
    assert(currStack->start == currStack->end);
}

void STStackDestory() {
    currStack->end = NULL;
    while (STStackPop());
    assert(!currStack->end && !currStack->start && !currTable);
}

bool STStackPush(SymTable *element) {
    assert(element);
    element->prev = currStack->start;
    currTable = currStack->start = element;
    return true;
}

bool STStackNewST() {
    currTable = (SymTable *)malloc(sizeof(SymTable));
    currTable->root = NULL, currTable->prev = NULL;
    return STStackPush(currTable);
}

bool STStackPop() {
    if (currStack->start == NULL) {
        if (currStack->end != NULL) assert(0);
        return false;
    }
    SymTable * tmp = currStack->start;
    currStack->start = tmp->prev;
    STDestory(&tmp);
    currTable = currStack->start;
    return true;
}

void STPrepare() {
    struTable = (SymTable *) malloc(sizeof(SymTable));
    funcTable = (SymTable *)malloc(sizeof(SymTable));
    currTable = (SymTable *)malloc(sizeof(SymTable));
    struTable->root = funcTable->root = currTable->root = NULL;
    struTable->prev = funcTable->prev = currTable->prev = NULL;
}

void STDestory(SymTable ** tmp) {
    assert(tmp != NULL);
    if ((*tmp) != NULL) { 
        RBDestroy(&((*tmp)->root), free);
        free(*tmp);
        (*tmp) = NULL;
    }
}

void STDestoryall() {
    STStackDestory();
    STDestory(&funcTable);
    STDestory(&struTable);
}

void STInsert(SymTable *st, char *name, SAType *type) {
    STElement * tmp = malloc(sizeof(STElement));
    tmp->name = name;
    tmp->num  = 0;
    tmp->ref = false;
    tmp->type = type;
    RBInsert(&(st->root), (void *)tmp, STECompare);
}
#define HELPER(x) \
    void x##STInsert(char *name, SAType *type) { \
        STInsert(x##Table, name, type); \
    }
HELPER(stru)
HELPER(func)
HELPER(curr)

STElement *STFind(SymTable *st, char *name) {
    STElement tmp;
    tmp.name = name;
    RBNode * result = RBSearch(&(st->root), (void *)(&tmp), STECompare);
    if (result != NULL && !STECompare((void *) result->value, (void *) (&tmp))) return (STElement *)result->value;
    else return NULL;
}
#define HELPER1(x) \
    STElement * x##STFind(char * name) { \
        return STFind(x##Table, name); \
    }
HELPER1(stru)
HELPER1(func)
STElement *currSTFind(char *name, bool thorough)
{
    SymTable * mid;
    mid = currTable;
    while (mid) {
        STElement * result = STFind(mid, name);
        if (result != NULL) return result;
        mid = mid->prev;
        if (!thorough) break;
    }
    return NULL;
}

STElement *STFindAll(char *name, bool thorough)
{
    STElement * ret = NULL;
    ret = struSTFind(name);
    if (ret == NULL) ret = funcSTFind(name);
    if (ret == NULL) ret = currSTFind(name, thorough);
    return ret;
}

int STECompare(const void *p1, const void *p2) {
    return strcmp(((STElement *)p1)->name, ((STElement *)p2)->name);
}

void STTranverse(SymTable *st, void (*handle)(void *)) {
    RBTranverse(&(st->root), handle);
}

void funcSTTranverse(void (*handle)(void *)) {
    STTranverse(funcTable, handle);
}