#ifndef STREE_H
#define STREE_H
#include "common.h"
struct SNode *initnode(const char *name, bool empty, int symbolnum, int line, int column);
void addchild(struct SNode * parent, struct SNode * child);
void dfsprint(struct SNode * cur, int depth);
void printtree();
extern FILE * fout;
#endif