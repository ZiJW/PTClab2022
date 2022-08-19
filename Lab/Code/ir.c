#include "ir.h"
#include "symtable.h"
#include "syntax.tab.h"

extern FILE * fout;
extern SAType * CONST_TYPE_INT, * CONST_TYPE_VOID;

IRCodeList allCode;

static unsigned int IRTempCount = 0;
static unsigned int IRLabelCount = 0;
static unsigned int IRVariableCount = 0;

static const IRCodeList STATIC_IR_EMPTY_CODELIST = {NULL, NULL};

void IRinit() {
    allCode.head = allCode.end = NULL;
    IRTempCount = IRLabelCount = IRVariableCount = 0;
}

IRCodePair IRTranslateExp(SNode *exp, IROperand place, bool deref) {
    assert(exp);
    IRCodePair ret = IRWrapCode(STATIC_IR_EMPTY_CODELIST, IRNewOperandNone(), NULL);
    SNode *e1 = exp->child;
    assert(e1);
    SNode *e2 = e1->next;
    SNode *e3 = e2 ? e2->next : NULL;
    if (e1->tokennum == INT) {
        IROperand op = IRNewOperandInstantINT(e1->ival);
        IRCode *code = IRNewCode(IR_CODE_ASSIGN);
        code->assign.left = place, code->assign.right = op;
        ret.list = IRAppendCode(ret.list, code);
    }
    else if (e1->tokennum == FLOAT) {
        IROperand op = IRNewOperandInstantFLOAT(e1->fval);
        IRCode *code = IRNewCode(IR_CODE_ASSIGN);
        code->assign.left = place, code->assign.right = op;
        ret.list = IRAppendCode(ret.list, code);
    }
    else if (e1->tokennum == ID && e2 == NULL)
    {
        if (place.kind == IR_OP_NONE)
            place = IRNewOperandTemp();
        IROperand op = IRNewOperandVariable(e1->idval, false);
        IRCode *code = IRNewCode(IR_CODE_ASSIGN);
        code->assign.left = place, code->assign.right = op;
        ret.list = IRAppendCode(ret.list, code);
        ret.type = currSTFind(e1->idval, true)->type;
        ret.op = op;
    }
    else if (e1->tokennum == ID)
    {
        if (place.kind == IR_OP_NONE) place = IRNewOperandTemp();
        if (e3->tokennum == RP) {
            if (strcmp(e1->idval, "read") == 0) {
                IRCode * code = IRNewCode(IR_CODE_READ);
                code->read.variable = place;
                ret.list = IRAppendCode(ret.list, code);
                ret.op = place;
            }
            else {
                IRCode *code = IRNewCode(IR_CODE_CALL);
                code->call.function = IRNewOperandFunction(e1->idval);
                code->call.result = place;
                ret.list = IRAppendCode(ret.list, code);
                ret.op = place;
            }
        }
        else {
            IRCodeList arglist = STATIC_IR_EMPTY_CODELIST;
            STElement * res = funcSTFind(e1->idval);
            IRCodeList list = IRTranslateArgs(e3, &arglist, res->type->function.parameter);
            ret.list = IRMergeCodeList(ret.list, list);
            if (strcmp(e1->idval, "write") == 0) {
                IRCode * code = IRNewCode(IR_CODE_WRITE);
                code->write.variable = arglist.head->arg.variable;
                ret.list = IRAppendCode(ret.list, code);
            }
            else {
                IRCode * code = IRNewCode(IR_CODE_CALL);
                code->call.result = place;
                code->call.function = IRNewOperandFunction(e1->idval);
                ret.list = IRMergeCodeList(ret.list, arglist);
                ret.list = IRAppendCode(ret.list, code);
                ret.op = place;
            }
        }
    }
    else if (e2 && e2->tokennum == LB)
    {
        IRCodePair pair = IRTranslateExp(e1, place, false);
        IROperand t1 = IRNewOperandTemp();
        IRCodePair pair2 = IRTranslateExp(e3, t1, true);
        ret.list = IRMergeCodeList(ret.list, pair.list);
        ret.list = IRMergeCodeList(ret.list, pair2.list);
        
        IRCode * code = IRNewCode(IR_CODE_MUL);
        code->binop.result = t1;
        code->binop.op1 = t1;
        code->binop.op2 = IRNewOperandInstantINT(pair.type->array.type->size);
        ret.list = IRAppendCode(ret.list, code);
        ret.type = pair.type->array.type;

        if (place.kind != IR_OP_NONE) {
            code = IRNewCode(IR_CODE_ADD);
            code->binop.result = place;
            code->binop.op1 = place;
            code->binop.op2 = t1;
            ret.list = IRAppendCode(ret.list, code);
        }

        if (deref && place.kind != IR_OP_NONE) {
            code = IRNewCode(IR_CODE_LOAD);
            code->load.left = place;
            code->load.right = place;
            ret.list = IRAppendCode(ret.list, code);
        }
    }
    else if (e2 && e2->tokennum == DOT)
    {
        assert(e3->tokennum == ID);
        IRCodePair pair = IRTranslateExp(e1, place, false);
        ret.list = IRMergeCodeList(ret.list, pair.list);
        SAType * type = pair.type;
        assert(type);
        SAFieldList * field;
        int offset = 0;
        for (field = type->structure; field != NULL; field = field->next) {
            if (strcmp(e3->idval, field->name) == 0) {
                type = field->type;
                break;
            }
            offset += field->type->size;
        }
        IRCode * code;
        ret.type = type;
        ret.op = place;
        if (offset > 0 && place.kind != IR_OP_NONE)
        {
            code = IRNewCode(IR_CODE_ADD);
            code->binop.result = place;
            code->binop.op1 = place;
            code->binop.op2 = IRNewOperandInstantINT(offset);
            ret.list = IRAppendCode(ret.list, code);
        }

        if (deref && place.kind != IR_OP_NONE)
        {
            code = IRNewCode(IR_CODE_LOAD);
            code->load.left = place;
            code->load.right = place;
            ret.list = IRAppendCode(ret.list, code);
        }
    }
    else if (e1->tokennum == MINUS)
    {
        IROperand t1 = IRNewOperandTemp();
        IRCodePair pair = IRTranslateExp(e2, t1, true);
        ret.list = IRMergeCodeList(ret.list, pair.list);
        if (place.kind != IR_OP_NONE) {
            IRCode * code = IRNewCode(IR_CODE_SUB);
            code->binop.result = place;
            code->binop.op1 = IRNewOperandInstantINT(0);
            code->binop.op2 = t1;
            ret.list = IRAppendCode(ret.list, code);
        }
    }
    else if (e1->tokennum == NOT)
    {
        ret.list = IRMergeCodeList(ret.list, IRTranslatePreCond(exp, place));
        ret.type = CONST_TYPE_INT;
    }
    else if (e1->tokennum == LP)
    {
        return IRTranslateExp(e2, place, deref);
    }
    else if (e2 && e2->tokennum == ASSIGNOP)
    {
        STElement * res = NULL;
        if (e1->child->tokennum == ID) {
            res = STFindAll(e1->child->idval, true);
        }
        if (e1->child->tokennum == ID && res && res->type->kind == BASIC) {
            IROperand v1 = IRNewOperandVariable(e1->child->idval, false);
            IROperand t1 = IRNewOperandTemp();
            IRCodePair pair2 = IRTranslateExp(e3, t1, true);
            ret.list = IRMergeCodeList(ret.list, pair2.list);

            IRCode * code = IRNewCode(IR_CODE_ASSIGN);
            code->assign.left = v1, code->assign.right = t1;
            ret.list = IRAppendCode(ret.list, code);

            if (place.kind != IR_OP_NONE) {
                code = IRNewCode(IR_CODE_ASSIGN);
                code->assign.left = place, code->assign.right = v1;
                ret.list = IRAppendCode(ret.list, code);
            }

            ret.op = t1;
            ret.type = pair2.type;
        }
        else {
            IROperand addr = IRNewOperandTemp();
            IROperand t1 = IRNewOperandTemp();
            IRCodePair pair = IRTranslateExp(e1, addr, false);
            if (pair.type->size == 4) {
                IRCode *code = IRNewCode(IR_CODE_SAVE);
                code->save.left = addr;
                code->save.right = t1;

                IRCodePair pair2 = IRTranslateExp(e3, t1, true);
                ret.list = IRMergeCodeList(ret.list, pair2.list);
                ret.list = IRMergeCodeList(ret.list, pair.list);
                ret.list = IRAppendCode(ret.list, code);

                if (place.kind != IR_OP_NONE) {
                    code = IRNewCode(IR_CODE_ASSIGN);
                    code->assign.left = place, code->assign.right = t1;
                    ret.list = IRAppendCode(ret.list, code);
                }

                ret.op = t1;
                ret.type = pair.type;
            }
            else {
                IRCodePair pair2 = IRTranslateExp(e3, t1, false);
                ret.list = IRMergeCodeList(ret.list, pair.list);
                ret.list = IRMergeCodeList(ret.list, pair2.list);
                //t1 = addr2

                IROperand iter = IRNewOperandTemp();
                IRCode * code = IRNewCode(IR_CODE_ASSIGN);
                code->assign.left = iter;
                code->assign.right = IRNewOperandInstantINT(0);
                ret.list = IRAppendCode(ret.list, code);
                //iter = #0;
                
                code = IRNewCode(IR_CODE_LABEL);
                IROperand label = IRNewOperandLabel();
                code->label.label = label;
                ret.list = IRAppendCode(ret.list, code);
                //label loop

                code = IRNewCode(IR_CODE_ADD);
                IROperand tmpaddr = IRNewOperandTemp();
                code->binop.result = tmpaddr;
                code->binop.op1 = addr;
                code->binop.op2 = iter;
                ret.list = IRAppendCode(ret.list, code);
                //tmpaddr = addr + iter

                code = IRNewCode(IR_CODE_LOAD);
                IROperand tmpval = IRNewOperandTemp();
                code->load.left = tmpval;
                code->load.right = t1;
                ret.list = IRAppendCode(ret.list, code);
                //tmpval = *t1;

                code = IRNewCode(IR_CODE_SAVE);
                code->load.left = tmpaddr;
                code->load.right = tmpval;
                ret.list = IRAppendCode(ret.list, code);
                //*tmpaddr = tmpval

                code = IRNewCode(IR_CODE_ADD);
                code->binop.result = t1;
                code->binop.op1 = t1;
                code->binop.op2 = IRNewOperandInstantINT(4);
                ret.list = IRAppendCode(ret.list, code);
                //t1 = t1 + 4

                code = IRNewCode(IR_CODE_ADD);
                code->binop.result = iter;
                code->binop.op1 = iter;
                code->binop.op2 = IRNewOperandInstantINT(4);
                ret.list = IRAppendCode(ret.list, code);
                //iter = iter + 4

                code = IRNewCode(IR_CODE_GOTO_RELOP);
                code->jmp_relop.op1 = iter;
                code->jmp_relop.op2 = IRNewOperandInstantINT(pair.type->size);
                code->jmp_relop.relop = IRNewOperandRelop(RELOP_L);
                code->jmp_relop.destination = label;
                ret.list = IRAppendCode(ret.list, code);
                //if iter < size goto loop
            }
        }
    }
    else if (e2 && (e2->tokennum == OR || e2->tokennum == AND || e2->tokennum == RELOP))
    {
        ret.list = IRMergeCodeList(ret.list, IRTranslatePreCond(exp, place));
        ret.type = CONST_TYPE_INT;
    }
    else if (e2)
    {
        IROperand t1 = IRNewOperandTemp(), t2 = IRNewOperandTemp();
        IRCodePair pair1 = IRTranslateExp(e1, t1, true);
        IRCodePair pair2 = IRTranslateExp(e3, t2, true);
        ret.list = IRMergeCodeList(pair1.list, pair2.list);
        IRCode * code;
        if (place.kind != IR_OP_NONE) {
            switch (e2->tokennum) {
                case PLUS : code = IRNewCode(IR_CODE_ADD); break;
                case MINUS : code = IRNewCode(IR_CODE_SUB); break;
                case STAR : code = IRNewCode(IR_CODE_MUL); break;
                case DIV : code = IRNewCode(IR_CODE_DIV); break;
                default : assert(0);
            }
            code->binop.result = place;
            code->binop.op1 = t1;
            code->binop.op2 = t2;
            ret.list = IRAppendCode(ret.list, code);
        }
    }
    return ret;
}

IRCodeList IRTranslateArgs(SNode *args, IRCodeList * arglist, SAFieldList * field) {
    SNode *e1 = args->child;
    assert(e1);
    SNode *e2 = e1->next;
    SNode *e3 = e2 ? e2->next : NULL;
    IRCodeList ret = STATIC_IR_EMPTY_CODELIST;
    IROperand t = IRNewOperandTemp();
    if (e2) {
        IRCodeList nxt = IRTranslateArgs(e3, arglist, field->next);
        ret = IRMergeCodeList(ret, nxt);
    }
    IRCode * code = IRNewCode(IR_CODE_ARG);
    code->arg.variable = t;
    *(arglist) = IRAppendCode(*(arglist), code);
    assert(field);
    IRCodeList cur = IRTranslateExp(e1, t, field->type->kind == BASIC).list;
    ret = IRMergeCodeList(ret, cur);
    return ret;
}

IRCodeList IRTranslateCompSt(SNode *compst) {
    assert(compst);
    assert(strcmp(compst->name, "CompSt") == 0);
    IRCodeList list1 = IRTranslateDefList(compst->child->next);
    IRCodeList list2 = IRTranslateStmtList(compst->child->next->next);
    return IRMergeCodeList(list1, list2);
}

IRCodeList IRTranslateDefList(SNode *deflist) {
    assert(strcmp(deflist->name, "DefList") == 0);
    if (deflist->empty) return STATIC_IR_EMPTY_CODELIST;
    IRCodeList ret = IRTranslateDef(deflist->child);
    return IRMergeCodeList(ret, IRTranslateDefList(deflist->child->next));
}

IRCodeList IRTranslateStmtList(SNode *stmtlist) {
    assert(stmtlist);
    assert(strcmp(stmtlist->name, "StmtList") == 0);
    IRCodeList ret = STATIC_IR_EMPTY_CODELIST;
    if (stmtlist->empty) {
        return ret;
    }
    ret = IRTranslateStmt(stmtlist->child);
    ret = IRMergeCodeList(ret, IRTranslateStmtList(stmtlist->child->next));
    return ret;
}

IRCodeList IRTranslateDef(SNode *def) {
    assert(def);
    assert(strcmp(def->name, "Def") == 0);
    return IRTranslateDecList(def->child->next);
}

IRCodeList IRTranslateDecList(SNode *declist) {
    assert(declist);
    assert(strcmp(declist->name, "DecList") == 0);
    IRCodeList ret = IRTranslateDec(declist->child);
    if (declist->child->next) {
        assert(declist->child->next->tokennum == COMMA);
        ret = IRMergeCodeList(ret, IRTranslateDecList(declist->child->next->next));
    }
    return ret;
}

IRCodeList IRTranslateDec(SNode *dec) {
    assert(dec);
    assert(strcmp(dec->name, "Dec") == 0);
    IRCodeList ret = STATIC_IR_EMPTY_CODELIST;
    SNode * vardec = dec->child;
    while (vardec && vardec->tokennum != ID) vardec = vardec->child;
    IROperand op = IRNewOperandVariable(vardec->idval, true);
    STElement * res = currSTFind(vardec->idval, true);
    if (res->type->kind != BASIC) {
        IRCode * tmpcode = IRNewCode(IR_CODE_DEC);
        tmpcode->dec.variable = op;
        tmpcode->dec.size = IRNewOperandInstantINT(res->type->size);
        ret = IRAppendCode(ret, tmpcode);
    }
    vardec = dec->child;
    if (vardec->next != NULL) {
        assert(vardec->next->tokennum == ASSIGNOP);
        IRCode * tmpcode = IRNewCode(IR_CODE_ASSIGN);
        tmpcode->assign.left = op;
        IROperand t = IRNewOperandTemp();
        tmpcode->assign.right = t;
        ret = IRMergeCodeList(ret, IRTranslateExp(vardec->next->next, t, true).list);
        ret = IRAppendCode(ret, tmpcode);
    }
    return ret;
}

IRCodeList IRTranslateStmt(SNode *stmt) {
    assert(stmt);
    assert(strcmp(stmt->name, "Stmt") == 0);
    IRCodeList ret = STATIC_IR_EMPTY_CODELIST;
    if (stmt->child->tokennum == IF) {
        SNode * iff = stmt->child, * exp = stmt->child->next->next, * stmt2 = stmt->child->next->next->next->next; 
        IROperand label_true = IRNewOperandLabel(), label_false = IRNewOperandLabel(), end = IRNewOperandLabel();
        ret = IRMergeCodeList(ret, IRTranslateCond(exp, label_true, label_false));

        IRCode * code = IRNewCode(IR_CODE_LABEL);
        code->label.label = label_true;
        ret = IRAppendCode(ret, code);
        
        ret = IRMergeCodeList(ret, IRTranslateStmt(stmt2));
        if (stmt2->next != NULL) {
            code = IRNewCode(IR_CODE_GOTO);
            code->jmp.destination = end;
            ret = IRAppendCode(ret, code);

            code = IRNewCode(IR_CODE_LABEL);
            code->label.label = label_false;
            ret = IRAppendCode(ret, code);

            SNode * stmt3 = stmt2->next->next;
            ret = IRMergeCodeList(ret, IRTranslateStmt(stmt3));

            code = IRNewCode(IR_CODE_LABEL);
            code->label.label = end;
            ret = IRAppendCode(ret, code);
        }
        else {
            code = IRNewCode(IR_CODE_LABEL);
            code->label.label = label_false;
            ret = IRAppendCode(ret, code);
        }
        return ret;
    }
    else if (stmt->child->tokennum == RETURN) {
        IRCode * tmp = IRNewCode(IR_CODE_RETURN);
        IROperand t1 = IRNewOperandTemp();
        ret = IRMergeCodeList(ret, IRTranslateExp(stmt->child->next, t1, true).list);
        tmp->ret.retval = t1;
        ret = IRAppendCode(ret, tmp);
        return ret;
    }
    else if (stmt->child->tokennum == WHILE) {
        SNode * whi = stmt->child, *exp = stmt->child->next->next, *stmt2 = stmt->child->next->next->next->next;
        IROperand begin = IRNewOperandLabel(), label_true = IRNewOperandLabel(), label_false = IRNewOperandLabel();
        IRCodeList list = IRTranslateCond(exp, label_true, label_false);

        IRCode * code = IRNewCode(IR_CODE_LABEL);
        code->label.label = begin;
        ret = IRAppendCode(ret, code);

        ret = IRMergeCodeList(ret, list);

        code = IRNewCode(IR_CODE_LABEL);
        code->label.label = label_true;
        ret = IRAppendCode(ret, code);

        ret = IRMergeCodeList(ret, IRTranslateStmt(stmt2));

        code = IRNewCode(IR_CODE_GOTO);
        code->jmp.destination = begin;
        ret = IRAppendCode(ret, code);

        code = IRNewCode(IR_CODE_LABEL);
        code->label.label = label_false;
        ret = IRAppendCode(ret, code);
        return ret;
    }
    else {
        if (strcmp(stmt->child->name, "CompSt") == 0) {
            ret.head = (IRCode *) stmt->child->codeliststart;
            ret.end = (IRCode *) stmt->child->codelistend;
            return ret;
        }
        else {
            assert(strcmp(stmt->child->name, "Exp") == 0);
            struct SNode *exp = stmt->child;
            return IRTranslateExp(exp, IRNewOperandNone(), false).list;
        }
    }
    return ret;
}

IRCodeList IRTranslatePreCond(SNode *exp, IROperand place) {
    IROperand label_true = IRNewOperandLabel(), label_false = IRNewOperandLabel();
    IRCodeList ret = STATIC_IR_EMPTY_CODELIST;

    if (place.kind != IR_OP_NONE) {
        IRCode * code = IRNewCode(IR_CODE_ASSIGN);
        code->assign.left = place;
        code->assign.right = IRNewOperandInstantINT(0);
        ret = IRAppendCode(ret, code);
    }

    IRCodeList list = IRTranslateCond(exp, label_true, label_false);
    ret = IRMergeCodeList(ret, list);

    IRCode *code = IRNewCode(IR_CODE_LABEL);
    code->label.label = label_true;
    ret = IRAppendCode(ret, code);

    if (place.kind != IR_OP_NONE) {
        code = IRNewCode(IR_CODE_ASSIGN);
        code->assign.left = place;
        code->assign.right = IRNewOperandInstantINT(1);
        ret = IRAppendCode(ret, code);
    }

    code = IRNewCode(IR_CODE_LABEL);
    code->label.label = label_false;
    ret = IRAppendCode(ret, code);
    return ret;
}

IRCodeList IRTranslateCond(SNode *exp, IROperand labeltrue, IROperand labelfalse) {
    assert(exp);
    assert(strcmp(exp->name, "Exp") == 0);
    IRCodeList ret = STATIC_IR_EMPTY_CODELIST;
    int testtokennum = 0;
    if (exp->child->next) testtokennum = exp->child->next->tokennum;
    if (exp->child->tokennum == NOT) {
        return IRTranslateCond(exp->child->next, labelfalse, labeltrue);
    } 
    else if (exp->child && exp->child->next && (testtokennum == RELOP || testtokennum == AND || testtokennum == OR)) {
        assert(exp->child); assert(exp->child->next);
        assert(exp->child->next->next);
        SNode * e1 = exp->child, * e2 = exp->child->next, * e3 = exp->child->next->next;
        switch (e2->tokennum) {
            case RELOP : {
                IROperand t1 = IRNewOperandTemp(), t2 = IRNewOperandTemp();
                IRCodePair pair1 = IRTranslateExp(e1, t1, true);
                IRCodePair pair2 = IRTranslateExp(e3, t2, true);
                IRCode * code = IRNewCode(IR_CODE_GOTO_RELOP);
                code->jmp_relop.op1 = t1, code->jmp_relop.op2 = t2;
                code->jmp_relop.relop = IRNewOperandRelop(e2->relop);
                code->jmp_relop.destination = labeltrue;
                ret = IRMergeCodeList(pair1.list, pair2.list);
                ret = IRAppendCode(ret, code);
                code = IRNewCode(IR_CODE_GOTO);
                code->jmp.destination = labelfalse;
                ret = IRAppendCode(ret, code);
                return ret;
            }
            case AND : {
                IROperand t1 = IRNewOperandLabel();
                IRCodeList list1 = IRTranslateCond(e1, t1, labelfalse);
                IRCodeList list2 = IRTranslateCond(e3, labeltrue, labelfalse);
                IRCode * code = IRNewCode(IR_CODE_LABEL);
                code->label.label = t1;
                ret = IRAppendCode(list1, code);
                ret = IRMergeCodeList(ret, list2);
                return ret;
            }
            case OR : {
                IROperand t1 = IRNewOperandLabel();
                IRCodeList list1 = IRTranslateCond(e1, labeltrue, t1);
                IRCodeList list2 = IRTranslateCond(e3, labeltrue, labelfalse);
                IRCode *code = IRNewCode(IR_CODE_LABEL);
                code->label.label = t1;
                ret = IRAppendCode(list1, code);
                ret = IRMergeCodeList(ret, list2);
                return ret;
            }
            default : assert(0);
        }
    }
    else {
        IROperand t1 = IRNewOperandTemp();
        IRCodePair pair1 = IRTranslateExp(exp, t1, true);
        IRCode *code = IRNewCode(IR_CODE_GOTO_RELOP);
        code->jmp_relop.op1 = t1; code->jmp_relop.op2 = IRNewOperandInstantINT(0);
        code->jmp_relop.relop = IRNewOperandRelop(RELOP_NEQ);
        code->jmp_relop.destination = labeltrue;
        ret = IRMergeCodeList(ret, pair1.list);
        ret = IRAppendCode(ret, code);
        code = IRNewCode(IR_CODE_GOTO);
        code->jmp.destination = labelfalse;
        ret = IRAppendCode(ret, code);
    }
    return ret;
}

void IRSAConnectionFunDec(SNode *fundec) {
    IRCodeList ret = STATIC_IR_EMPTY_CODELIST;
    IRCode *code = IRNewCode(IR_CODE_FUNCTION);
    code->function.function.kind = IR_OP_FUNCTION;
    code->function.function.name = fundec->child->idval;
    ret = IRAppendCode(ret, code);

    char *name = fundec->child->idval;
    SAType * type = funcSTFind(name)->type;
    SAFieldList * field = type->function.parameter;
    //printf("fuck\n");
    //printf("%s\n", fundec->child->idval);
    //show();
    for (SAFieldList * fd = field; fd != NULL; fd = fd->next) {
        code = IRNewCode(IR_CODE_PARAM);
        code->param.variable = IRNewOperandVariable(fd->name, false);
        ret = IRAppendCode(ret, code);
    }
    fundec->codeliststart = (void *) ret.head;
    fundec->codelistend = (void *) ret.end;
}

void IRSAConnectionCompSt(SNode *compst) {
    SNode *deflist = compst->child->next;
    assert(deflist);
    assert(strcmp(deflist->name, "DefList") == 0);
    SNode *stmtlist = deflist->next;
    assert(stmtlist);
    assert(strcmp(stmtlist->name, "StmtList") == 0);

    IRCodeList list1 = STATIC_IR_EMPTY_CODELIST, list2 = STATIC_IR_EMPTY_CODELIST;
    list1.head = (IRCode *) deflist->codeliststart;
    list1.end = (IRCode *)  deflist->codelistend;
    list2.head = (IRCode *) stmtlist->codeliststart;
    list2.end = (IRCode *) stmtlist->codelistend;

    IRCodeList list = IRMergeCodeList(list1, list2);
    compst->codeliststart = (void *) list.head;
    compst->codelistend = (void *) list.end;
}

void IRSAConnectionDefList(SNode *deflist) {
    assert(deflist);
    assert(strcmp(deflist->name, "DefList") == 0);
    IRCodeList list = IRTranslateDefList(deflist);
    deflist->codeliststart = (void *)list.head;
    deflist->codelistend = (void *)list.end;
}

void IRSAConnectionStmtList(SNode *stmtlist) {
    assert(stmtlist);
    assert(strcmp(stmtlist->name, "StmtList") == 0);
    IRCodeList list = IRTranslateStmtList(stmtlist);
    stmtlist->codeliststart = (void *)list.head;
    stmtlist->codelistend = (void *)list.end;
}

void IRSAGenerateCode(SNode *fundec, SNode *compst) {
    IRCodeList list;
    list.head = (IRCode *) fundec->codeliststart;
    list.end = (IRCode *) fundec->codelistend;
    allCode = IRMergeCodeList(allCode, list);

    list.head = (IRCode *) compst->codeliststart;
    list.end = (IRCode *) compst->codelistend;
    allCode = IRMergeCodeList(allCode, list);

    IRCode * code = IRNewCode(IR_CODE_RETURN);
    code->ret.retval = IRNewOperandInstantINT(0);
    allCode = IRAppendCode(allCode, code);
}

IROperand IRNewOperandNone() {
    IROperand ret;
    ret.kind = IR_OP_NONE;
    ret.size = 0;
    return ret;
}

IROperand IRNewOperandTemp() {
    IROperand ret;
    ret.kind = IR_OP_TEMP;
    ret.size = 4;
    ret.num = ++IRTempCount;
    return ret;
}

IROperand IRNewOperandLabel() {
    IROperand ret;
    ret.kind = IR_OP_LABEL;
    ret.size = 0;
    ret.num = ++IRLabelCount;
    return ret;
}

IROperand IRNewOperandVariable(char * name, bool dec) {
    STElement * res = STFindAll(name, true);
    assert(res);
    if (res->num == 0) res->num = ++IRVariableCount;
    IROperand ret;
    if (res->type->kind == BASIC) {
        ret.size = 4;
        ret.num = res->num;
        ret.kind = IR_OP_VARIABLE;
    }
    else {
        ret.size = res->type->size;
        ret.num = res->num;
        if (dec) {
            ret.kind = IR_OP_VARIABLE;
            res->ref = true;
        }
        else if (res->ref == false) ret.kind = IR_OP_VARIABLE;
        else ret.kind = IR_OP_ADDRESS;
    }
    return ret;
}

IROperand IRNewOperandInstantINT(int tmp) {
    IROperand ret;
    ret.size = 4;
    ret.kind = IR_OP_INSTANT_INT;
    ret.ivalue = tmp;
    return ret;
}

IROperand IRNewOperandInstantFLOAT(float tmp) {
    IROperand ret;
    ret.size = 4;
    ret.kind = IR_OP_INSTANT_FLOAT;
    ret.fvalue = tmp;
    return ret;
}

IROperand IRNewOperandFunction(char *name) {
    STElement * res = funcSTFind(name);
    assert(res);
    assert(res->type->size == 4);
    IROperand ret;
    ret.size = 4;
    ret.kind = IR_OP_FUNCTION;
    ret.name = name;
    return ret;
}

IROperand IRNewOperandRelop(enum RELOP_STATE relop) {
    IROperand ret;
    ret.size = 0;
    ret.kind = IR_OP_RELOP;
    ret.relop = relop;
    return ret;
}

IRCode * IRNewCode(enum IRCodeType kind) {
    IRCode * ret = (IRCode *) malloc(sizeof(struct IRCode));
    ret->kind = kind;
    ret->prev = ret->next = NULL;
    return ret; 
}

IRCodeList IRAppendCode(IRCodeList list, IRCode *code) {
    if (list.head == NULL) {
        assert(list.end == NULL);
        list.head = code;
        list.end = code;
        assert(list.head == code && list.end == code);
    }
    else {
        list.end->next = code;
        code->prev = list.end;
        list.end = code;
    }
    return list;
}

IRCodeList IRMergeCodeList(IRCodeList lista, IRCodeList listb) {
    if (lista.head == NULL) {
        assert(lista.end == NULL);
        return listb;
    }
    if (listb.head == NULL) {
        assert(listb.end == NULL);
        return lista;
    }
    lista.end->next = listb.head;
    listb.head->prev = lista.end;
    lista.end = listb.end;
    return lista;
}

IRCodeList IRRemoveCode(IRCodeList list, IRCode *code) {
    if (code == list.head) {
        list.head = list.head->next;
        if (list.end == code) list.end = NULL;
        free(code);
        return list;
    }
    else if (code == list.end) {
        list.end = code->prev;
        list.end->next = NULL;
    }
    else {
        code->prev->next = code->next;
        code->next->prev = code->prev;
    }
    free(code);
    return list;
}

IRCodePair IRWrapCode(IRCodeList list, IROperand op, SAType *type) {
    IRCodePair pair;
    pair.list = list;
    pair.op   = op;
    pair.type = type;
    return pair;
}

void IRDestoryCode(IRCodeList list) {
    for (IRCode * cur = list.head; cur != NULL; cur = cur->next) {
        if (cur->prev) cur->prev->next = NULL;
        if (cur->next) cur->next->prev = NULL;
        if (cur == list.end) {
            free(cur);
            break;
        }
        else free(cur);
    }
}

void IRFreshCode(IRCodeList list) {
    allCode = IRMergeCodeList(allCode, list);
}

size_t IROutputOperand(char *s, IROperand * op) {
    switch (op->kind)
    {
    case IR_OP_TEMP:
        return sprintf(s, "t%u", op->num);
    case IR_OP_LABEL: 
        return sprintf(s, "label%u", op->num);
    case IR_OP_INSTANT_INT: 
        return sprintf(s, "#%d", op->ivalue);
    case IR_OP_INSTANT_FLOAT: 
        return sprintf(s, "#%f", op->fvalue);
    case IR_OP_ADDRESS:
        return sprintf(s, "&v%u", op->num);
    case IR_OP_VARIABLE: 
        return sprintf(s, "v%u", op->num);
    case IR_OP_FUNCTION:
        return sprintf(s, "%s", op->name);
    case IR_OP_RELOP: {
        switch (op->relop) {
            case RELOP_G : 
                return sprintf(s, ">");
            case RELOP_L:
                return sprintf(s, "<");
            case RELOP_GE:
                return sprintf(s, ">=");
            case RELOP_LE:
                return sprintf(s, "<=");
            case RELOP_EQ:
                return sprintf(s, "==");
            case RELOP_NEQ:
                return sprintf(s, "!=");
            default: 
                assert(0);
        }
    }
    default:
        return sprintf(s, "NONE");
    }
}

size_t IROutputCode(char *s, IRCode *code) {
    char * rec = s;
    switch (code->kind) {
        case IR_CODE_LABEL : {
            s += sprintf(s, "LABEL ");
            s += IROutputOperand(s, &code->label.label);
            s += sprintf(s, " :");
            break;
        }
        case IR_CODE_FUNCTION : {
            //assert(code->function.function.kind == IR_OP_FUNCTION);
            s += sprintf(s, "FUNCTION ");
            s += IROutputOperand(s, &code->function.function);
            s += sprintf(s, " :");
            break;
        }
        case IR_CODE_GETADDR: {
            s += IROutputOperand(s, &code->getaddr.left);
            s += sprintf(s, " := &");
            s += IROutputOperand(s, &code->getaddr.right);
            break;
        }
        case IR_CODE_ASSIGN : {
            s += IROutputOperand(s, &code->assign.left);
            s += sprintf(s, " := ");
            s += IROutputOperand(s, &code->assign.right);
            break;
        }
        case IR_CODE_ADD :
        case IR_CODE_SUB :
        case IR_CODE_MUL : 
        case IR_CODE_DIV : {
            s += IROutputOperand(s, &code->binop.result);
            s += sprintf(s, " := ");
            s += IROutputOperand(s, &code->binop.op1);
            s += sprintf(s, " ");
            char op;
            switch (code->kind) {
            case IR_CODE_ADD: op = '+'; break;
            case IR_CODE_SUB: op = '-'; break;
            case IR_CODE_MUL: op = '*'; break;
            case IR_CODE_DIV: op = '/'; break;
            default: assert(0);
            }
            s += sprintf(s, "%c ", op);
            s += IROutputOperand(s, &code->binop.op2);
            break;
        }
        case IR_CODE_LOAD : {
            s += IROutputOperand(s, &code->load.left);
            s += sprintf(s, " := *");
            s += IROutputOperand(s, &code->load.right);
            break;
        }
        case IR_CODE_SAVE : {
            s += sprintf(s, "*");
            s += IROutputOperand(s, &code->save.left);
            s += sprintf(s, " := ");
            s += IROutputOperand(s, &code->load.right);
            break;
        }
        case IR_CODE_GOTO : {
            s += sprintf(s, "GOTO ");
            s += IROutputOperand(s, &code->jmp.destination);
            break;
        }
        case IR_CODE_GOTO_RELOP : {
            s += sprintf(s, "IF ");
            s += IROutputOperand(s, &code->jmp_relop.op1);
            s += sprintf(s, " ");
            s += IROutputOperand(s, &code->jmp_relop.relop);
            s += sprintf(s, " ");
            s += IROutputOperand(s, &code->jmp_relop.op2);
            s += sprintf(s, " GOTO ");
            s += IROutputOperand(s, &code->jmp_relop.destination);
            break;
        }
        case IR_CODE_RETURN : {
            s += sprintf(s, "RETURN ");
            s += IROutputOperand(s, &code->ret.retval);
            break;
        }
        case IR_CODE_DEC : {
            s += sprintf(s, "DEC ");
            s += IROutputOperand(s, &code->dec.variable);
            s += sprintf(s, " ");
            s += sprintf(s, "%u", code->dec.size.ivalue);
            break;
        }
        case IR_CODE_ARG : {
            s += sprintf(s, "ARG ");
            s += IROutputOperand(s, &code->arg.variable);
            break;
        }
        case IR_CODE_CALL : {
            s += IROutputOperand(s, &code->call.result);
            s += sprintf(s, " := CALL ");
            s += IROutputOperand(s, &code->call.function);
            break;
        }
        case IR_CODE_PARAM : {
            s += sprintf(s, "PARAM ");
            s += IROutputOperand(s, &code->param.variable);
            break;
        }
        case IR_CODE_READ : {
            s += sprintf(s, "READ ");
            s += IROutputOperand(s, &code->read.variable);
            break;
        }
        case IR_CODE_WRITE : {
            s += sprintf(s, "WRITE ");
            s += IROutputOperand(s, &code->write.variable);
            break;
        }
        default : {
            //printf("%d\n", code->kind);
            assert(0);
        }
    }
    return s - rec;
}

#define MAX_ONE_LINE_CODE_LENGTH 512
char OneLineCode[MAX_ONE_LINE_CODE_LENGTH];
void IROutputCodeList(IRCodeList *list) {
    char * s;
    IRCode * cur = list->head;
    if (cur == NULL) return;
    #ifdef IRDEBUG
    int i = 0;
    do {
        i += 1;
        s = OneLineCode;
        s += IROutputCode(s, cur);
        fprintf(fout, "%s\n", OneLineCode);
        fprintf(fout, "%p %p %p\n", cur->prev, cur, cur->next);
        cur = cur->next;
    } while (cur && cur->prev != list->end && i <= 10);
    #else
    do {
        s = OneLineCode;
        s += IROutputCode(s, cur);
        fprintf(fout, "%s\n", OneLineCode);
        cur = cur->next;
    } while (cur && cur->prev != list->end);
    #endif
}

void IROutput() {
    IROutputCodeList(&allCode);
}