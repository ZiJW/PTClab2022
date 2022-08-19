#include "type.h"
#include "syntax.tab.h"
#include "symtable.h"
#include "semantic.h"
#include "ir.h"
SAType _CONST_TYPE_INT, _CONST_TYPE_FLOAT, _CONST_TYPE_VOID;
SAType * CONST_TYPE_INT, * CONST_TYPE_FLOAT,  * CONST_TYPE_VOID;
SAFieldList CONST_FIELD_INT;
void SATypeInit() {
    CONST_TYPE_VOID = &(_CONST_TYPE_VOID);
    CONST_TYPE_VOID->kind = VOID;
    CONST_TYPE_VOID->parent = CONST_TYPE_VOID;
    CONST_TYPE_VOID->size = -1;
    
    CONST_TYPE_INT = &(_CONST_TYPE_INT);
    CONST_TYPE_INT->kind = BASIC;
    CONST_TYPE_INT->basic = INT;
    CONST_TYPE_INT->parent = CONST_TYPE_INT;
    CONST_TYPE_INT->size = 4;

    CONST_TYPE_FLOAT = &(_CONST_TYPE_FLOAT);
    CONST_TYPE_FLOAT->kind = BASIC;
    CONST_TYPE_FLOAT->basic = FLOAT;
    CONST_TYPE_FLOAT->parent = CONST_TYPE_FLOAT;
    CONST_TYPE_FLOAT->size = 4;

    CONST_FIELD_INT.next = NULL;
    CONST_FIELD_INT.type = CONST_TYPE_INT;
}

//handle read and write function in ir part
void ReadWriteInit() {
    SAType * write = GetNewSAType();
    write->kind = FUNCTION;
    write->size = 4;
    write->function.defined = true;
    write->function.line = 0;
    write->function.returntype = CONST_TYPE_INT;
    //write->function.parameter = &CONST_FIELD_INT;
    SAFieldList * field = (SAFieldList *) malloc(sizeof(SAFieldList));
    field->name = "write_only_param";
    field->type = CONST_TYPE_INT;
    field->next = NULL;
    write->function.parameter = field;
    funcSTInsert("write", write);

    SAType *read = GetNewSAType();
    read->kind = FUNCTION;
    read->size = 4;
    read->function.defined = true;
    read->function.line = 0;
    read->function.returntype = CONST_TYPE_INT;
    read->function.parameter = NULL;
    funcSTInsert("read", read);
    assert(funcSTFind("read") != NULL);
}

void SAHandleProgram(struct SNode *pro) {
    assert(pro);
    assert(strcmp(pro->name, "Program") == 0);
    SAHandleExtDefList(pro->child);
}

void SAHandleExtDefList(struct SNode *extdeflist) {
    assert(extdeflist);
    assert(strcmp(extdeflist->name, "ExtDefList") == 0);
    if (extdeflist->empty) return;
    SAHandleExtDef(extdeflist->child);
    SAHandleExtDefList(extdeflist->child->next);
}

void SAHandleExtDef(struct SNode *extdef) {
    assert(extdef);
    assert(strcmp(extdef->name, "ExtDef") == 0);
    struct SNode * body = extdef->child->next;
    SAType * type = SAHandleSpecifier(extdef->child);
    if (body->tokennum == SEMI) return;
    if (strcmp(body->name, "ExtDecList") == 0) {
        SAHandleExtDecList(body, type);
    }
    else {
        if (body->next->tokennum == SEMI) {
            STStackNewST();
            SAHandleFunDec(body, type, false);
            STStackPop();
        }
        else {
            FunctionReturnType = type;
            STStackNewST();
            SAHandleFunDec(body, type, true);
            IRSAConnectionFunDec(body); // the entry to IR part
            SAHandleCompSt(body->next, false);
            IRSAGenerateCode(body, body->next);
            STStackPop();
        }
    }
}

void SAHandleExtDecList(struct SNode *extdec, SAType *type) {
    assert(extdec);
    assert(strcmp(extdec->name, "ExtDecList") == 0);
    SAHandleVarDec(extdec->child, type, true, false, false, false);
    struct SNode * comma = extdec->child->next;
    if (comma != NULL) {
        assert(comma->tokennum == COMMA);
        SAHandleExtDecList(comma->next, type);
    }
}

SAType *SAHandleSpecifier(struct SNode *spe) {
    assert(spe);
    assert(strcmp(spe->name, "Specifier") == 0);
    assert(spe->child);
    struct SNode *tmp = spe->child;
    if (tmp->tokennum == TYPE) {
        if (strcmp(tmp->idval, "int") == 0) return CONST_TYPE_INT;
        if (strcmp(tmp->idval, "float") == 0) return CONST_TYPE_FLOAT;
        assert(0);
    }
    else {
        return SAHandleStructSpecifier(tmp);
    }
}

SAType *SAHandleStructSpecifier(struct SNode *stru) {
    assert(stru);
    assert(strcmp(stru->name, "StructSpecifier") == 0);
    assert(stru->child->tokennum == STRUCT);
    struct SNode * tmp = stru->child->next;
    char * name = NULL;
    if (strcmp(tmp->name, "OptTag") == 0) {
        if (!tmp->empty) {
            name = tmp->child->idval;
        }
        tmp = tmp->next;
        assert(tmp && tmp->tokennum == LC);
        STStackNewST();
        SAFieldList * res = SAHandleDefList(tmp->next, true);
        STStackPop();
        tmp = tmp->next->next;
        assert(tmp && tmp->tokennum == RC);
        struct SAType * ret = GetNewSAType();
        ret->kind = STRUCTURE;
        SAFieldList * rec = res;
        ret->size = 0;
        while (rec) {
            ret->size += rec->type->size;
            rec = rec->next;
        }
        //TODO size
        ret->structure = res;
        if (name != NULL) {
           STElement * fres = STFindAll(name, true);
           if (fres != NULL) {
               ThrowError(SAError_Duplicated_Struct_Name, stru->line, name, NULL);
               return NULL;
           }
           struSTInsert(name, ret);
        }
        return ret;
    }
    else {
        assert(strcmp(tmp->name, "Tag") == 0);
        name = tmp->child->idval;
        STElement *fres = struSTFind(name);
        if (fres == NULL) {
            ThrowError(SAError_Undefined_Struct_Name, stru->line, name, NULL);
            return NULL;
        }
        return struSTFind(name)->type;
    }
    return NULL;
}

SAFieldList * SAHandleVarDec(struct SNode *vardec, SAType *type, bool inST,bool return_or_not, bool stru, bool func) {
    assert(vardec);
    assert(strcmp(vardec->name, "VarDec") == 0);
    struct SNode * tmp = vardec->child;
    assert(tmp);
    if (tmp->tokennum == ID) {
        STElement *fres = STFindAll(tmp->idval, false);
        if (fres != NULL) {
            if (stru) ThrowError(SAError_Redefined_Field, tmp->line, tmp->idval, NULL);
            else ThrowError(SAError_Redefined_Variable, tmp->line, tmp->idval, NULL);
            return NULL;
        }
        if (inST) currSTInsert(tmp->idval, type);
        if (return_or_not) {
            SAFieldList *ret = (SAFieldList *) malloc(sizeof(SAFieldList));
            ret->name = tmp->idval;
            ret->type = type;
            ret->next = NULL;
            return ret;
        }
    }
    else {
        assert(strcmp(tmp->name, "VarDec") == 0);
        assert(tmp->next->tokennum == LB);
        assert(tmp->next->next->tokennum == INT);
        assert(tmp->next->next->next->tokennum == RB);
        struct SNode *num = tmp->next->next;
        SAType * ret = GetNewSAType();
        ret->kind = ARRAY;
        ret->size = num->ival * type->size;//TODO
        ret->array.size = num->ival;
        ret->array.kind = type->kind;
        ret->array.type = type;
        return SAHandleVarDec(tmp, ret, inST, return_or_not, stru, func);
    }
    return NULL;
}

bool SAHandleFunDec(struct SNode *funcdec, SAType *type, bool define) {
    assert(funcdec);
    assert(strcmp(funcdec->name, "FunDec") == 0);
    struct SNode * nam = funcdec->child;
    assert(nam->tokennum == ID);
    assert(nam->next->tokennum == LP);
    STElement * tmp;
    tmp = currSTFind(nam->idval, true);
    if (tmp == NULL) tmp = struSTFind(nam->idval);
    if (tmp != NULL) {
        ThrowError(0, nam->line, nam->idval, NULL); //TODO if the function name is already usded, handle it as the default error. TBD
        return false;
    }
    tmp = funcSTFind(nam->idval);
    if (tmp == NULL) {
        SAType *res = GetNewSAType();
        res->kind = FUNCTION;
        res->size = 4; //TODO;
        res->function.line = nam->line;
        res->function.defined = define;
        res->function.returntype = type;
        struct SNode *arg = nam->next->next;
        if (arg->tokennum == RP)
        {
            res->function.parameter = NULL;
            funcSTInsert(nam->idval, res);
        }
        else
        {
            res->function.parameter = SAHandleVarList(arg, true);
            if (res->function.parameter != NULL) funcSTInsert(nam->idval, res);
        }
    }
    else {
        SAType * res = tmp->type;
        assert(res->kind == FUNCTION);
        struct SNode *arg = nam->next->next;
        SAFieldList * parameter;
        if (arg->tokennum == RP)
            parameter = NULL;
        else
            parameter = SAHandleVarList(arg, true);
        bool test = res->function.returntype == type && SACompareField(res->function.parameter, parameter);
        //only two or more definition are forbidened
        if (res->function.defined == true && define == true) {
            ThrowError(SAError_Redefined_Function, nam->line, nam->idval, NULL);
            return false;
        }
        else {
            // Declearing function many times are allowed, 
            // but only the definition decides return type and push parameter into stack
            if (!test) {
                ThrowError(SAError_Inconsist_Declearation_Function, nam->line, nam->idval, NULL);
                return false;
            }
            res->function.defined = define;
        }
    }
    return true;
}

SAFieldList *SAHandleVarList(struct SNode *varlist, bool define) {
    assert(varlist);
    assert(strcmp(varlist->name, "VarList") == 0);
    struct SNode * child = varlist->child;
    assert(child);
    if (child->next == NULL) return SAHandleParamDec(child, define);
    else {
        assert(child->next);
        assert(child->next->tokennum == COMMA);
        SAFieldList * ret = SAHandleParamDec(child, define);
        if (ret == NULL) return NULL;
        ret->next = SAHandleVarList(child->next->next, define);
        if (ret->next == NULL) return NULL;
        return ret;
    }
}

SAFieldList *SAHandleParamDec(struct SNode *paramdec, bool define) {
    assert(paramdec);
    assert(strcmp(paramdec->name, "ParamDec") == 0);
    SAFieldList * ret 
        = SAHandleVarDec(paramdec->child->next, SAHandleSpecifier(paramdec->child), define, true, false, true);
    //only when function is defined should the parameters be pushed into stack
    //assert(ret);
    return ret;
}

void SAHandleCompSt(struct SNode *cmpst, bool newstack) {
    assert(cmpst);
    assert(strcmp(cmpst->name, "CompSt") == 0);
    assert(cmpst->child->tokennum == LC);
    assert(cmpst->child->next->next->next->tokennum == RC);
    SNode *deflist = cmpst->child->next;
    assert(deflist);
    assert(strcmp(deflist->name, "DefList") == 0);
    SNode *stmtlist = deflist->next;
    assert(stmtlist);
    assert(strcmp(stmtlist->name, "StmtList") == 0);

    if (newstack) STStackNewST();
    SAHandleDefList(deflist, false);
    IRSAConnectionDefList(deflist);
    SAHandleStmtList(stmtlist);
    IRSAConnectionStmtList(stmtlist);
    IRSAConnectionCompSt(cmpst);
    if (newstack) STStackPop();
}

void SAHandleStmtList(struct SNode *stmtlist) {
    assert(stmtlist);
    assert(strcmp(stmtlist->name, "StmtList") == 0);
    if (stmtlist->empty) return;
    else {
        SAHandleStmt(stmtlist->child);
        SAHandleStmtList(stmtlist->child->next);
    }
}
void SAHandleStmt(struct SNode *stmt) {
    assert(stmt);
    assert(strcmp(stmt->name, "Stmt") == 0);
    if (stmt->child->tokennum == IF) {
        assert(stmt->child->next->tokennum == LP);
        struct SNode *exp = stmt->child->next->next;
        assert(exp->next->tokennum == RP);
        SAType *tmp = SAHandleExp(exp);
        if (tmp && !SACompareType(tmp, CONST_TYPE_INT))
            ThrowError(SAError_Mismatched_Operate_Type, exp->line, NULL, "if ()");
        struct SNode * nstmt = exp->next->next;
        SAHandleStmt(nstmt);
        if (nstmt->next != NULL) {
            assert(nstmt->next->tokennum == ELSE);
            SAHandleStmt(nstmt->next->next);
        }
    }
    else if (stmt->child->tokennum == RETURN){
        SAType * res = SAHandleExp(stmt->child->next);
        assert(stmt->child->next->next->tokennum == SEMI);
        if (res && !SACompareType(res, FunctionReturnType)) {
            ThrowError(SAError_Mismatched_Rerturn_Type, stmt->line, NULL, NULL);
            return;
        }
    }
    else if (stmt->child->tokennum == WHILE)
    {
        assert(stmt->child->next->tokennum == LP);
        struct SNode *exp = stmt->child->next->next;
        assert(exp->next->tokennum == RP);
        SAType *tmp = SAHandleExp(exp);
        if (tmp && !SACompareType(tmp, CONST_TYPE_INT))
            ThrowError(SAError_Mismatched_Operate_Type, exp->line, NULL, "while ()");
        struct SNode *nstmt = exp->next->next;
        SAHandleStmt(nstmt);
    }
    else {
        if (strcmp(stmt->child->name, "CompSt") == 0) {
            SAHandleCompSt(stmt->child, true);
        }
        else {
            assert(strcmp(stmt->child->name, "Exp") == 0);
            struct SNode * exp = stmt->child;
            SAHandleExp(exp);
            assert(exp->next->tokennum == SEMI);
        }
    }
}

SAFieldList *SAHandleDefList(struct SNode *deflist, bool stru) {
    assert(deflist);
    assert(strcmp(deflist->name, "DefList") == 0);
    if (deflist->empty) {
        return NULL;
    }
    else {
        SAFieldList *cur = SAHandleDef(deflist->child, stru);
        SAFieldList *nxt = SAHandleDefList(deflist->child->next, stru);
        SAFieldList *ret = cur;
        if (stru) {
            if (cur) {
                while (cur->next) cur = cur->next;
                cur->next = nxt;
                return ret;
            }
            else return nxt;
        }
        else return NULL;
    }
}

SAFieldList *SAHandleDef(struct SNode *def, bool stru) {
    assert(def);
    assert(strcmp(def->name, "Def") == 0);
    SAType * type = SAHandleSpecifier(def->child);
    return SAHandleDecList(def->child->next, type, stru);
}

SAFieldList *SAHandleDecList(struct SNode *declist, SAType * type, bool stru) {
    assert(declist);
    assert(strcmp(declist->name, "DecList") == 0);
    if (declist->child->next != NULL) {
        assert(declist->child->next->tokennum == COMMA);
        SAFieldList * res1 = SAHandleDec(declist->child, type, stru);
        SAFieldList * res2 = SAHandleDecList(declist->child->next->next, type, stru);
        if (stru && res1) res1->next = res2;
        if (res1) return res1;
        else return res2;
    }
    else {
        return SAHandleDec(declist->child, type, stru);
    }
}

SAFieldList *SAHandleDec(struct SNode *dec, SAType *type, bool stru) {
    assert(dec);
    assert(strcmp(dec->name, "Dec") == 0);
    if (dec->child->next != NULL) {
        assert(dec->child->next->tokennum == ASSIGNOP);
        assert(dec->child->next->next);
        struct SNode * vardec = dec->child, * exp = dec->child->next->next;
        SAFieldList *ret = SAHandleVarDec(vardec, type, true, true, stru, false);
        //whatever it require return or not, field is needed here to check type
        SAType * exptype = SAHandleExp(exp);
        if (stru)
        {
            //only the struct field need return, where assignop is not allowed
            //thus this must be error, throw error and return NULL;
            //but the reminder info should be "="? //TODO
            ThrowError(SAError_Redefined_Field, dec->child->next->line, "=", NULL);
            return ret;
        }
        if (exptype == NULL) return NULL;
        if (ret && !SACompareType(ret->type, exptype)) {
            ThrowError(SAError_Mismatched_Assignop_Type, exp->line, NULL, NULL);
            return NULL;
        }
        return ret;
    }
    else return SAHandleVarDec(dec->child, type, true, true, stru, false);
}

SAType *SAHandleExp(struct SNode *exp) {
    assert(exp);
    assert(strcmp(exp->name, "Exp") == 0);
    struct SNode * e1 = exp->child;
    assert(e1);
    struct SNode * e2 = e1->next;
    struct SNode * e3 = e2 ? e2->next : NULL;
    if (e1->tokennum == INT) {
        return CONST_TYPE_INT;
    }
    else if (e1->tokennum == FLOAT) {
        return CONST_TYPE_FLOAT;
    }
    else if (e1->tokennum == ID && e2 == NULL) {
        STElement * res = currSTFind(e1->idval, true);
        if (res == NULL) res = funcSTFind(e1->idval);
        if (res == NULL) {
            ThrowError(SAError_Undefined_Variable, e1->line, e1->idval, NULL);
            return NULL;
        }
        return res->type;
    }
    else if (e1->tokennum == ID) {
        STElement * res = STFindAll(e1->idval, true);
        if (res == NULL) {
            ThrowError(SAError_Undefined_Function, e1->line, e1->idval, NULL);
            return NULL;
        }
        if (res->type->kind != FUNCTION) {
            ThrowError(SAError_Not_Function, e1->line, e1->idval, NULL);
            return NULL;
        }
        struct SNode * func = e1;
        assert(func->next);
        assert(func->next->tokennum == LP);
        if (func->next->next->tokennum == RP) {
            if (!SACompareField(NULL, res->type->function.parameter))
            {
                ThrowError(SAError_Wrong_Function_Argument, func->line, func->idval, NULL);
                return NULL;
            }
            return res->type->function.returntype;
        }
        else {
            struct SNode *arg = func->next->next;
            assert(arg->next->tokennum == RP);
            SAFieldList * argfield = SAHandleArgs(arg);
            if (!argfield) return NULL;
            if (!SACompareField(argfield, res->type->function.parameter)) {
                ThrowError(SAError_Wrong_Function_Argument, arg->line, func->idval, NULL);
                return NULL;
            }
            return res->type->function.returntype;
        }
    }
    else if (e2 && e2->tokennum == LB) {
        SAType * res = SAHandleExp(e1);
        if (res == NULL) return NULL;
        if (res->kind != ARRAY) {
            ThrowError(SAError_Not_Array, e1->line, NULL, NULL);
            return NULL;
        }
        else {
            struct SNode * stru = e1;
            struct SNode * posi = stru->next->next;
            assert(posi->next->tokennum == RB);
            SAType * par = SAHandleExp(posi);
            if (par == NULL) return NULL;
            if (par->kind == BASIC && par->basic == INT) {
                return res->array.type;
            }
            else {
                ThrowError(SAError_NonInteger_Array_Access, posi->line, NULL, NULL);
                return NULL;
            }
        }
    }
    else if (e2 && e2->tokennum == DOT) {
        struct SNode *id = e2->next;
        assert(id->tokennum == ID);
        SAType * res = SAHandleExp(e1);
        if (res == NULL) return NULL;
        if (res->kind != STRUCTURE) {
            ThrowError(SAError_Illigal_Use_DOT, e2->line, NULL, ".");
            return NULL;
        }
        SAFieldList * tmp = res->structure;
        while (tmp && strcmp(tmp->name, id->idval) != 0) tmp = tmp->next;
        if (tmp) return tmp->type;
        else {
            assert(tmp == NULL);
            ThrowError(SAError_Nonexsist_Field, id->line, NULL, id->idval);
            return NULL;
        }
    }
    else if (e1->tokennum == MINUS) {
        SAType * ret = SAHandleExp(e2);
        if (ret == NULL) return NULL;
        if (SACompareType(ret, CONST_TYPE_INT) || SACompareType(ret, CONST_TYPE_FLOAT)) {
            return ret;
        }
        else {
            ThrowError(SAError_Mismatched_Operate_Type, e2->line, NULL, NULL);
            return NULL;
        }
    }
    else if (e1->tokennum == NOT) {
        SAType * ret = SAHandleExp(e2);
        //only integer can be operated by not
        if (ret == NULL) return NULL;
        if (SACompareType(ret, CONST_TYPE_INT)) {
            return ret;
        }
        else {
            ThrowError(SAError_Mismatched_Operate_Type, e2->line, NULL, NULL);
            return NULL;
        }
    }
    else if (e1->tokennum == LP) {
        assert(e2->next->tokennum == RP);
        return SAHandleExp(e2);
    }
    else if (e2 && e2->tokennum == ASSIGNOP) {
        struct SNode * newexp = e1->child;
        bool test = (newexp->tokennum == ID && newexp->next == NULL)|| (newexp->next != NULL && newexp->next->tokennum == LB) || (newexp->next != NULL && newexp->next->tokennum == DOT);
        //the check of left hand variable is prior to the type consistency
        if (!test) {
            ThrowError(SAError_Left_Hand_Not_Variable, e1->line, NULL, NULL);
            return NULL;
        }
        SAType * res1 = SAHandleExp(e1), * res2 = SAHandleExp(e3);
        if (res1 == NULL || res2 == NULL) return NULL;
        if (SACompareType(res1, res2)) {
            return res1;
        }
        else {
            ThrowError(SAError_Mismatched_Assignop_Type, e1->line, NULL, NULL);
            return NULL;
        }
    }
    else if (e2 && (e2->tokennum == OR || e2->tokennum == AND)) {
        SAType * res1 = SAHandleExp(e1),  * res2 = SAHandleExp(e3);
        if (res1 == NULL || res2 == NULL) return NULL;
        if (SACompareType(res1, CONST_TYPE_INT) && SACompareType(res2, CONST_TYPE_INT)){
            return CONST_TYPE_INT;
        }
        ThrowError(0, e2->line, NULL, NULL);
        return NULL;
    }
    else if (e2) {
        int tk = e2->tokennum;
        assert(tk == RELOP || tk == DIV || tk == STAR || tk == PLUS || tk == MINUS);
        SAType * res1 = SAHandleExp(e1), *res2 = SAHandleExp(e3);
        if (res1 == NULL || res2 == NULL) return NULL;
        if (!SACompareType(res1, res2)) {
            ThrowError(SAError_Mismatched_Operate_Type, e2->line, NULL, NULL);
            return NULL;
        }
        if (tk == RELOP) return CONST_TYPE_INT;
        if (SACompareType(res1, CONST_TYPE_INT)) return CONST_TYPE_INT;
        if (SACompareType(res1, CONST_TYPE_FLOAT)) return CONST_TYPE_FLOAT;
        ThrowError(0, e2->line, NULL, NULL);
        return NULL;
    }
    assert(0);
}

SAFieldList *SAHandleArgs(struct SNode *args) {
    assert(args);
    assert(strcmp(args->name, "Args") == 0);
    struct SNode * e1 = args->child;
    struct SNode * e2 = e1->next;
    struct SNode * e3 = e2 ? e2->next : NULL;
    if (e2) {
        assert(e2->tokennum == COMMA);
        SAFieldList * ret = (SAFieldList*) malloc(sizeof(SAFieldList));
        ret->type = SAHandleExp(e1);
        if (!ret->type) return NULL;
        ret->next = SAHandleArgs(e3);
        if (ret->next)
            return ret;
        else return NULL;
    }
    else {
        SAFieldList *ret = (SAFieldList *)malloc(sizeof(SAFieldList));
        ret->type = SAHandleExp(e1);
        if (!ret->type) return NULL;
        ret->next = NULL;
        return ret;
    }
}

bool SACompareType(SAType *a, SAType *b) {
    if (!a || !b) {
        #ifdef SADEBUG
        printf("Meet empty SAType while conducting semantic analysis\n");
        #endif
        return false;
    }
    a = GetParentSAType(a), b = GetParentSAType(b);
    if (a->kind != b->kind) return false;
    if (a->parent == b->parent) return true;
    switch (a->kind) {
        case BASIC : return a->basic == b->basic;
        case ARRAY : return SACompareType(a->array.type, b->array.type);
        case STRUCTURE : if (SACompareField(a->structure, b->structure)) {
            b->parent = a;
            return true;
        } else return false;
        case FUNCTION : if (SACompareType(a->function.returntype, b->function.returntype) && SACompareField(a->function.parameter, b->function.parameter)) {
            b->parent = a;
            return true;
        }
        else return false;
        default : return false;
    }
}

bool SACompareField(SAFieldList *a, SAFieldList *b) {
    if (!a && !b) {
        return true;
    }
    else if (!a || !b) {
        #ifdef SADEBUG
        printf("Meet empty SAFieldList while conducting semantic analysis\n");
        #endif
        return false;
    }
    return SACompareType(a->type, b->type) && SACompareField(a->next, b->next);
}

SAType *GetNewSAType()
{
    SAType *ret = (SAType *)malloc(sizeof(SAType));
    ret->parent = ret;
    return ret;
}

SAType *GetParentSAType(SAType *tmp) {
    if (tmp->parent == tmp) return tmp;
    else return tmp->parent = GetParentSAType(tmp->parent);
}

void showField(SAFieldList *a)
{
    SAFieldList *cur = a;
    printf("SAFieldList : \n");
    while (cur != NULL)
    {
        printf("name : %s  ", cur->name);
        printf("%p ", cur->type);
        cur = cur->next;
        printf("%p\n", cur);
    }
    printf("\n");
}

void show() {
    if (funcSTFind("gcd2"))
        showField(funcSTFind("gcd2")->type->function.parameter);
}
