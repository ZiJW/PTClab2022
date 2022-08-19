#include "asm.h"

extern IRCodeList allCode;
extern FILE * fout;

struct RBNode * funcNameRoot;

const char *ASMheader =
    ".data\n"
    "_prompt: .asciiz \"Enter an integer:\"\n"
    "_ret: .asciiz \"\\n\"\n"
    ".globl main\n"
    ".text\n"
    "read:\n"
    "  li $v0, 4\n"
    "  la $a0, _prompt\n"
    "  syscall\n"
    "  li $v0, 5\n"
    "  syscall\n"
    "  jr $ra\n"
    "\n"
    "write:\n"
    "  li $v0, 1\n"
    "  syscall\n"
    "  li $v0, 4\n"
    "  la $a0, _ret\n"
    "  syscall\n"
    "  jr $ra\n";

const char * regs[] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "fp", "$ra"
};

#define t0_ regs[8]
#define t1_ regs[9]
#define v0_ regs[2]
#define a0_ regs[4]

void IR2ASM() {
    ASMPrepare();
    ASMTransList();
}

void ASMPrepare() {
    IRCode * cur = allCode.head;
    IRCode * next;
    for (;cur != NULL && cur->prev != allCode.end;) {
        if (cur->kind == IR_CODE_FUNCTION) {
            next = ASMPrepareFunction(cur, &cur->function.root);
            ASMFuncTableInsert(&funcNameRoot, &cur->function.function);
            cur = next;
        }
        else
            cur = cur->next;
    }
    for (;cur != NULL && cur->prev != allCode.end;) {
        if (cur->kind == IR_CODE_CALL) {
            IROperand * res = ASMFuncTableFind(&funcNameRoot, &cur->call.function);
            if (res) {
                //not the read or write function, otherwise the res will be null
                cur->call.function.size = res->size;
            }
        }
    }
}

IRCode *ASMPrepareFunction(IRCode *start, RBNode **root) {
    assert(start->kind == IR_CODE_FUNCTION);
    IRCode * cur;
    int size = 8, args = 0;
    for (cur = start->next; cur != NULL && cur->kind != IR_CODE_FUNCTION; cur = cur->next) {
        switch (cur->kind) {
            case IR_CODE_ASSIGN :
            case IR_CODE_GETADDR :
            case IR_CODE_LOAD :
            case IR_CODE_SAVE: {
                size += ASMRecordVariable(&cur->assign.left, root, size);
                size += ASMRecordVariable(&cur->assign.right, root, size);
                break;
            }
            case IR_CODE_ADD :
            case IR_CODE_SUB :
            case IR_CODE_MUL :
            case IR_CODE_DIV : {
                size += ASMRecordVariable(&cur->binop.result, root, size);
                size += ASMRecordVariable(&cur->binop.op1, root, size);
                size += ASMRecordVariable(&cur->binop.op2, root, size);
                break;
            }
            case IR_CODE_GOTO_RELOP : {
                size += ASMRecordVariable(&cur->jmp_relop.op1, root, size);
                size += ASMRecordVariable(&cur->jmp_relop.op2, root, size);
                break;
            }
            case IR_CODE_RETURN :
                size += ASMRecordVariable(&cur->ret.retval, root, size);
                break;
            case IR_CODE_DEC :
            //view as address while counting the size
                cur->dec.variable.kind = IR_OP_ADDRESS;
                size += ASMRecordVariable(&cur->dec.variable, root, size);
                break;
            case IR_CODE_READ: 
            case IR_CODE_WRITE : {
                size += ASMRecordVariable(&cur->read.variable, root, size);
                break;
            }
            case IR_CODE_CALL : {
                size += ASMRecordVariable(&cur->call.result, root, size);
                break;
            }
            case IR_CODE_ARG : {
                size += ASMRecordVariable(&cur->arg.variable, root, size);
                break;
            }
            case IR_CODE_PARAM : {
                int parasize = ASMRecordVariable(&cur->param.variable, root, 0);
                cur->param.variable.offset = 0 - args;
                args += 4;
                break;
            }
            default: break;
        }
    }
    start->function.function.size = size;
    return cur;
}

int ASMRecordVariable(IROperand *op, RBNode **root, int offset) {
    switch (op->kind) {
        case IR_OP_TEMP :
        case IR_OP_VARIABLE :
        case IR_OP_ADDRESS : {
            if (ASMVariaTableContains(root, op)) {
                IROperand * res = ASMVariaTableFind(root, op);
                op->offset = res->offset;
                return 0;
            }
            else {
                op->offset = offset + op->size;
                ASMVariaTableInsert(root, op);
                //printf("%d %d %d\n", op->kind, op->num, op->offset);
                return op->size;
            }
        }
        default:
        break;
    }
    return 0;
}

int ASMOperandCompare(const void *a, const void *b) {
    IROperand *t1 = (IROperand *)a, *t2 = (IROperand *) b;
    if (t1->kind != t2->kind) return t1->kind > t2->kind ? 1 : -1;
    else if (t1->num == t2->num) return 0;
    else return t1->num > t2->num ? 1 : -1;
}

int ASMFuncNameCompare(const void *a, const void *b) {
    IROperand *t1 = (IROperand *)a, *t2 = (IROperand *)b;
    return strcmp(t1->name, t2->name);
}

void ASMVariaTableInsert(RBNode **root, IROperand *op) {
    RBInsert(root, (void *) op, ASMOperandCompare);
}

IROperand *ASMVariaTableFind(RBNode **root, IROperand *op) {
    RBNode * res = RBSearch(root, (void *) op, ASMOperandCompare);
    return (IROperand *) res->value;
}

bool ASMVariaTableContains(RBNode **root, IROperand *op) {
    return RBContains(root, (void *) op, ASMOperandCompare);
}

void ASMFuncTableInsert(RBNode **root, IROperand *op) {
    RBInsert(root, (void *)op, ASMFuncNameCompare);
}

IROperand *ASMFuncTableFind(RBNode **root, IROperand *op) {
    RBNode *res = RBSearch(root, (void *)op, ASMFuncNameCompare);
    return (IROperand *)res->value;
}

void ASMTransList() {
    fprintf(fout, "%s", ASMheader);
    for (IRCode * cur = allCode.head; cur != NULL && cur->prev != allCode.end; cur = cur->next) {
        ASMTransCode(cur);
    }
    for (IRCode * cur = allCode.head; cur != NULL && cur->prev != allCode.end; cur = cur->next) {
        if (cur->kind == IR_CODE_FUNCTION) RBDestroy(&cur->function.root, NULL);
    }
}

int ASMArgsSize = 0;
int ASMFuncSize = 0;
void ASMTransCode(IRCode *code) {
    switch (code->kind) {
        case IR_CODE_FUNCTION: {
            ASMFuncSize = code->function.function.size;
            assert(code);
            if (strcmp(code->function.function.name, "main") == 0)
                fprintf(fout, "\n%s:\n", code->function.function.name);
            else
                fprintf(fout, "\nfunction_wrap_%s:\n", code->function.function.name);
            fprintf(fout, "  sw $ra, -4($sp)\n");
            fprintf(fout, "  sw $fp, -8($sp)\n");
            fprintf(fout, "  move $fp, $sp\n");
            fprintf(fout, "  subu $sp, $sp, %d\n", code->function.function.size);
            break;
        }
        case IR_CODE_ASSIGN: {
            ASMRegLoad(t0_, code->assign.right);
            ASMRegSave(t0_, code->assign.left);
            break;
        }
        case IR_CODE_ADD : {
            ASMRegLoad(t0_, code->binop.op1);
            ASMRegLoad(t1_, code->binop.op2);
            fprintf(fout, "  add %s, %s, %s\n", t0_, t0_, t1_);
            ASMRegSave(t0_, code->binop.result);
            break;
        }
        case IR_CODE_SUB : {
            ASMRegLoad(t0_, code->binop.op1);
            ASMRegLoad(t1_, code->binop.op2);
            fprintf(fout, "  sub %s, %s, %s\n", t0_, t0_, t1_);
            ASMRegSave(t0_, code->binop.result);
            break;
        }
        case IR_CODE_MUL : {
            ASMRegLoad(t0_, code->binop.op1);
            ASMRegLoad(t1_, code->binop.op2);
            fprintf(fout, "  mul %s, %s, %s\n", t0_, t0_, t1_);
            ASMRegSave(t0_, code->binop.result);
            break;
        }
        case IR_CODE_DIV : {
            ASMRegLoad(t0_, code->binop.op1);
            ASMRegLoad(t1_, code->binop.op2);
            fprintf(fout, "  div %s, %s, %s\n", t0_, t0_, t1_);
            fprintf(fout, "  mflo %s\n", t0_);
            ASMRegSave(t0_, code->binop.result);
            break;
        }
        case IR_CODE_LOAD : {
            ASMRegLoad(t0_, code->load.right);
            fprintf(fout, "  lw %s, 0(%s)\n", t1_, t0_);
            ASMRegSave(t1_, code->load.left);
            break;
        }
        case IR_CODE_SAVE : {
            ASMRegLoad(t0_, code->save.right);
            ASMRegLoad(t1_, code->save.left);
            fprintf(fout, "  sw %s, 0(%s)\n", t0_, t1_);
            break;
        }
        case IR_CODE_LABEL: {
            fprintf(fout, "label%d:\n", code->label.label.num);
            break;
        }
        case IR_CODE_GOTO : {
            fprintf(fout, "  j label%d\n", code->jmp.destination.num);
            break;
        }
        case IR_CODE_GOTO_RELOP : {
            ASMRegLoad(t0_, code->jmp_relop.op1);
            ASMRegLoad(t1_, code->jmp_relop.op2);
            switch (code->jmp_relop.relop.relop) {
            case RELOP_EQ:
                fprintf(fout, "  beq %s, %s, label%d\n", t0_, t1_, code->jmp_relop.destination.num);
                break;
            case RELOP_NEQ:
                fprintf(fout, "  bne %s, %s, label%d\n", t0_, t1_, code->jmp_relop.destination.num);
                break;
            case RELOP_G:
                fprintf(fout, "  bgt %s, %s, label%d\n", t0_, t1_, code->jmp_relop.destination.num);
                break;
            case RELOP_L:
                fprintf(fout, "  blt %s, %s, label%d\n", t0_, t1_, code->jmp_relop.destination.num);
                break;
            case RELOP_GE:
                fprintf(fout, "  bge %s, %s, label%d\n", t0_, t1_, code->jmp_relop.destination.num);
                break;
            case RELOP_LE:
                fprintf(fout, "  ble %s, %s, label%d\n", t0_, t1_, code->jmp_relop.destination.num);
                break;
            default: 
                break;
            }
            break;
        }
        case IR_CODE_READ : {
            fprintf(fout, "  jal read\n");
            ASMRegSave(v0_, code->read.variable);
            break;
        }
        case IR_CODE_WRITE: {
            ASMRegLoad(a0_, code->write.variable);
            fprintf(fout, "  jal write\n");
            break;
        }
        case IR_CODE_CALL : {
            if (strcmp(code->call.function.name, "main") == 0)
                fprintf(fout, "  jal %s\n", code->call.function.name);
            else
                fprintf(fout, "  jal function_wrap_%s\n", code->call.function.name);
            ASMRegSave(v0_, code->call.result);
            assert(ASMFuncSize != 0);
            fprintf(fout, "  addiu $sp, $sp, %d\n", ASMArgsSize);
            ASMArgsSize = 0;
            break;
        }
        case IR_CODE_RETURN : {
            ASMRegLoad(v0_, code->ret.retval);
            fprintf(fout, "  lw $ra, -4($fp)\n");
            fprintf(fout, "  lw $fp, -8($fp)\n");
            fprintf(fout, "  addiu $sp, $sp, %d\n", ASMFuncSize);
            fprintf(fout, "  jr $ra\n");
            break;
        }
        case IR_CODE_ARG: {
            ASMRegLoad(t0_, code->arg.variable);
            fprintf(fout, "  addiu $sp, $sp, -4\n");
            fprintf(fout, "  sw %s, 0($sp)\n", t0_);
            ASMArgsSize += 4;
            break;
        }
        default: 
        break;
    }
}

void ASMRegLoad(const char *reg, IROperand op) {
    if (op.kind == IR_OP_INSTANT_INT) 
        fprintf(fout, "  li %s, %d\n", reg, op.ivalue);
    else {
        if (op.kind == IR_OP_ADDRESS) 
            fprintf(fout, "  la %s, %d($fp)\n", reg, -op.offset);
        else if (op.kind == IR_OP_VARIABLE || op.kind == IR_OP_TEMP)
            fprintf(fout, "  lw %s, %d($fp)\n", reg, -op.offset);
        else assert(0);
    }
}

void ASMRegSave(const char *reg, IROperand op) {
    fprintf(fout, "  sw %s, %d($fp)\n", reg, -op.offset);
}