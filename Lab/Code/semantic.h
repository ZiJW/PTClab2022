#ifndef SEMANTIC_H
#define SEMANTIC_H
#include "common.h"
#include "type.h"
#include "symtable.h"
enum SAErrorType
{
    SAError_Undefined_Variable = 1,
    SAError_Undefined_Function,
    SAError_Redefined_Variable,
    SAError_Redefined_Function,
    SAError_Mismatched_Assignop_Type,
    SAError_Left_Hand_Not_Variable,
    SAError_Mismatched_Operate_Type,
    SAError_Mismatched_Rerturn_Type,
    SAError_Wrong_Function_Argument,
    SAError_Not_Array,
    SAError_Not_Function,
    SAError_NonInteger_Array_Access,
    SAError_Illigal_Use_DOT,
    SAError_Nonexsist_Field,
    SAError_Redefined_Field,
    SAError_Duplicated_Struct_Name,
    SAError_Undefined_Struct_Name,
    SAError_Undefined_Decleared_Function,
    SAError_Inconsist_Declearation_Function
};
void SemanticAnalysis();
void SemanticCheck();
void CheckFunctionDefined(void * x);
void ThrowError(enum SAErrorType num, int line, char * msg1, char * msg2);
#endif