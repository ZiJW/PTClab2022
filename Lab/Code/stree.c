#include "stree.h"
#include "syntax.tab.h"
struct SNode *initnode(const char *name, bool empty, int symbolnum, int line, int column)
{
    struct SNode * tmp = malloc(sizeof(struct SNode));
    tmp->empty = empty;
    tmp->name = name;
    tmp->symbolnum = symbolnum;
    tmp->line = line,
    tmp->column = column;
    tmp->tokennum = -1;
    tmp->relop = RELOP_NONE;
    tmp->child = NULL, tmp->next = NULL;
    return tmp;
}
void addchild(struct SNode * parent, struct SNode * child)
{
    #ifdef BIDEBUG
    //printf("%p %s, %p %s\n", parent, parent->name, child, child->name);
    #endif
    if (parent->child == NULL) {
        parent->child = child;
    }
    else {
        struct SNode * tmp = parent->child;
        while (tmp->next != NULL) {
            tmp = tmp->next;
        }
        tmp->next = child;
    }
}

void dfsprint(struct SNode *cur, int depth) {
    if (cur->empty == false) {
        for (int i = 0; i < depth; i++)
        {
            fprintf(fout, "  ");
        }
        fprintf(fout, "%s", cur->name);
        if (cur->tokennum != -1)
        {
            switch (cur->tokennum)
            {
            case INT:
                fprintf(fout, ": %d", cur->ival);
                break;
            case FLOAT:
                fprintf(fout, ": %f", cur->fval);
                break;
            case TYPE:
            case ID:
                fprintf(fout, ": %s", cur->idval);
                break;
            }
        }
    #ifdef BIDEBUG
        printf(" (%d)", (int)cur->line);
    #else
        else
        {
            fprintf(fout, " (%d)", (int)cur->line);
        }
    #endif
        fprintf(fout, "\n");
    }
    if (cur->child != NULL) dfsprint(cur->child, depth + 1);
    if (cur->next != NULL) dfsprint(cur->next, depth);
}