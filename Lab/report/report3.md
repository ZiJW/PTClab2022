# L3实验报告

191250145 王子鉴

## 0 实验说明

编译方法：make后在当前文件夹下编译出.`/parser`文件，接受两个参数

> `./parser input_file output_file`

`input_file`为输入文件，`output_file`为输出文件

## 1 新增文件及接口

`ir.h/c`生成中间代码接口及定义

总体思路是在检查完一部分代码生成相应的中间代码。具体接口如下

```c
void IRSAConnectionFunDec(SNode * fundec);
void IRSAConnectionCompSt(SNode * compst);
void IRSAConnectionDefList(SNode *deflist);
void IRSAConnectionStmtList(SNode *stmtlist);
void IRSAGenerateCode(SNode *fundec, SNode * compst);
```

说明

1. 因为实验二选用的是functional style，所以在退出一个`compst`的时候，因为要`pop`掉其符号表，在其内部变量无法查找到，所以在每次退出`compst`的检查之前，就生成其代码，并在语法树上记录
2. 但仍然要分成两部分处理: `DefList`和`StmtList`。因为在`CompSt`里声明数组或结构体局部变量的时候，需要生成`dec`中间代码，其前后操作数的类型要发生改变(从一个变量变为一个需要取地址的变量)，而如果`StmtList`中有内层的`CompSt`使用了这个数组，此时操作数仍为改变前的类型。所以先处理`DefList`, 再处理`StmtList`， 对应的`IRSAConnectionDefList`与`IRSAConnectionStmtList`生成代码，而`IRSAConnectionCompSt`只是将它们结合在一起。
3. 除此之外，只需对函数定义进行处理即可，`IRSAConnectionFunDec`对`FunDec`生成中间代码，`IRSAGenerateCode`将函数声明和函数体的中间代码串联起来，并添加到全局中间代码链表中。

## 2 数据结构

原有数据结构改变

```C
struct SNode {
    //...
    void * codeliststart, * codelistend; //存储中间代码的首尾地址
};
```

```C
typedef struct STElement {
    //...
    bool ref; //如果需要dec一段内存，则dec前为false， dec后为true，并根据此来将操作数的类型从变量改为取地址变量
} STElement;
```

## 3 其他说明

对数组/结构体的赋值，伪代码如下

```c
addr = GetAddrFromExp(exp1);
t1 = GetAddrFromExp(exp2);
iter = 0;
LABEL label:
tmpval = *t1;
tmpaddr = addr + iter;
*tmpaddr = tmpval;
t1 = t1 + 4;
iter = iter + 4;
IF iter < addr.type.size GOTO label;
```

`exp`结点的处理麻烦一点

```C
IRCodePair IRTranslateExp(SNode * exp, IROperand place, bool deref);
```

其中deref表示是否需要解引用，`codepair`是将一段中间代码和类型检查中的类型包在一起，方便数组和结构体的处理

本次实验还没涉及到优化。

## 4 问题与致谢

感谢张天昀学长。一开始思考了很久不知道该怎么设计接口能处理数组和结构体，所以本次实验借鉴了张天昀学长的中间代码生成的接口，做了一些修改再用。相比有些同学写完了发现接口设计不好没法处理，还是提前想明白细节更省事。[代码链接](https://github.com/doowzs/PTC2020-Labs/blob/master/Code/ir.h)

感谢孙伟杰同学在较大测试用例的debug时的指导。