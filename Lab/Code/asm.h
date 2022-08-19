#include "common.h"
#include "ir.h"

void IR2ASM();

void ASMPrepare();
IRCode * ASMPrepareFunction(IRCode * start, RBNode ** root);
int ASMRecordVariable(IROperand * op, RBNode ** root, int offset);
int ASMFuncNameCompare(const void *a, const void *b);
int ASMOperandCompare(const void * a, const void * b);
void ASMVariaTableInsert(RBNode ** root, IROperand * op);
IROperand * ASMVariaTableFind(RBNode **root, IROperand * op);
bool ASMVariaTableContains(RBNode **root, IROperand *op);
void ASMFuncTableInsert(RBNode **root, IROperand *op);
IROperand *ASMFuncTableFind(RBNode **root, IROperand *op);

void ASMTransList();
void ASMTransCode(IRCode * code);

void ASMRegLoad(const char * reg, IROperand op);
void ASMRegSave(const char *reg, IROperand op);