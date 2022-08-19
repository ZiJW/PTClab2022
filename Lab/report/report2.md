# Lab2 实验报告

## 0 说明

编译和上次文件一样，在Code 文件夹下，运行make命令即可 编译出./parser文件，参数为输入文件的相对路径或绝对路径。

项目中没说清楚的一些错误：

1. 当if 和while的条件exp返回的不是int时，逻辑运算参数不为int类型时，比较运算参数不为int/float时，报错第七种
2. 函数声明可以有多次，但必须要一致
3. 函数不管是声明还是定义，都以第一次为准

## 1 文件结构

```shell
.
├── Makefile
├── common.h
├── lexical.l
├── main.c
├── rbtree.c			#红黑树
├── rbtree.h
├── semantic.c			#语义分析入口
├── semantic.h
├── stree.c		
├── stree.h
├── symtable.c			#符号表(symbol table)
├── symtable.h
├── syntax.y
├── type.c				#类型定义
└── type.h
```



## 2 数据结构

选用底层数据结构：红黑树

最开始想用linux内核中的rbtree.c he rbtree.h文件，但我本地的实验环境为mac，在尝试本地复制linux/rbtree.h, linux/rbtree.h文件后，需要container_of 的定义，而container_of 需要\_\_same\_type的定义，\_\_same\_type需要kernel的内置函数，发现不太可行。

在ubuntu服务器上试着include <linux/rbtree.h>, 仍然无法找到，所以最后选用了别人已经写好的rbtree。

在建立了红黑树后，进一步封装。因为底层数据结构采用的是红黑树，所以自然选择Functional Style

。所以一个符号表包括一个红黑树的根节点和一个只想前一个符号表的指针

```C
typedef struct SymTable {
    struct RBNode * root;
    struct SymTable * prev;
} SymTable;
```

而符号表栈只需维护最开始的和结束的符号表即可

```C
typedef struct SymTabStack {
    struct SymTable * start;
    struct SymTable * end;
} SymTabStack;
```

而SAType(Semantic Analysis Type) 则需要包含如下信息

```C
typedef struct SAType
{
    enum SABasicType kind;				//指明类型
    size_t size;						//大小，后面的生成中间代码计算偏移量可能会用到
    struct SAType * parent;				//后补充的，用来做并查集，加速嵌套结构体的比较
    union
    {
        int basic;					//当类型为basic时，basic为int和float在lexical中分配到的值
        struct
        {
            int size;				//大小
            enum SABasicType kind;	//元素的类型
            struct SAType *type;	//指向类型的指针
        } array;					//数组类型
        SAFieldList *structure;		//结构体，用一个指向FieldList的指针来表示
        struct
        {
            bool defined;		//定义为true, 声明为false
            int line;			//行数，在分析结束的时候，如果有声明但未定义的函数，需要指出行号
            struct SAType *returntype;		//返回值
            struct SAFieldList *parameter;	//参数，与结构体类似
        } function;					//函数	
    };
} SAType;
```

总共有三个符号表，两个是struTable,  funcTable, 分别存储指向结构体和函数的符号表， currTable则是当前作用域的符号表，并且只有currTable构成栈。

在进入一个CompSt作用域的时候，只有函数需要提前压栈并需要创建一个新的符号表，其他的只需要在处理CompSt节点时处理即可。在处理结构体时，遇到{}压栈即可(这样方便域名的检查)。



## 3 处理过程

```C
void SemanticAnalysis() {
    SATypeInit();
    STPrepare();
    STStackPrepare();				//预处理
    SAHandleProgram(SyntaxTreeRoot);//从根节点开始调用处理函数，对每一个节点进行分析
    SemanticCheck();				//扫描函数符号表，有没有声明但没定义的函数
    STDestoryall();					//回收符号表
}
```

## 4 错误处理

统一调用ThrowError函数来报错，并且提供必要的信息。

因为所有返回值都是一个指针(除了bool返回值判断是否有错)，报错后，统一返回空指针，相当于返回了一个统一的报错指针，并且在节点处理过程中，遇到空指针就跳过。也有SAFieldList指针为空，表示这个域为空，分情况处理即可。

## 5 实验感想

一开始并不清楚到底要做什么事情，在这里感谢金前程同学的指点。

留下了一个潜在的问题。处理函数定义的时候，是先压栈再插入参数，和实际过程并不太符合。可能在生成中间代码的时候需要再次修改。

## 6 压力测试

如果有结构体`A0, A1`... `An`,并且有如下关系

```c
struct A0 { int a, b; };
struct A1 { struct A0 a, b; };
struct A2 { struct A1 a, b; };
//....
```

那么朴素的比较`An`是否相等，时间复杂度为$O(2^n)$，所以最后在结构中加上`parent`，用并查集的方法来加速过程。
