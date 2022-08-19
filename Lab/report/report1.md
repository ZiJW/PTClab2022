# Lab1 实验报告

## 1. 文件说明

```bash
.
├── Makefile
├── common.h			#定义语法树结点数据类型，调试信息的开关，和全局的数据（如ID最长长度）
├── lexical.l			#.l文件，词法分析部分
├── main.c				#main文件，传入文件参数，并执行yyparse
├── stree.c				#树的函数的实现
├── stree.h				#树的函数的声明
└── syntax.y			#.y文件，语法分析部分
```

## 2. 数据类型

```C
struct SNode {
    const char * name;			//语法单元的名称
    bool empty;					//是否为空
    ll line, column;			//行和列
    int tokennum, symbolnum;	//词法单元序号和语法单元序号
    enum RELOP_STATE relop;		//RELOP表示的比较序列
    union						//值，包括int，float，id的名字
    {
        unsigned int ival;
        float fval;
        char idval[MAXIDLENGTH];
    };
    struct SNode * child, * next;//树相关指针
};
```

## 2. 词法分析

在`common.h`中定义`LEXDEBUG`的话，每扫描到一个词法单元都会打印出名字，位置和值

`yylex`的返回值为指向一个`SNode`的指针，每次扫描到一个合法的词法单元就申请一个SNode大小的空间，根据扫描到的内容填充tokennum，val和relop的状态

## 3. 语法分析

在`common.h`中定义`BIDEBUG`则开启调试，某个产生式符合规则时则输出其行号，并进行下一步的处理

在语法分析部分定义了三个宏，`DEFAULT`, `DEFAULT_EMPTY`和`DEFAULT_ERROR`, 是处理一般情况、产生式右部为空串和错误恢复的方法。默认的处理方法是创建结点，并把地址赋值给$$(yyval), 初始化(语法序号，是否为空，名称，行号)，再把产生式右部所有节点添加为此节点的孩子，如果孩子是终结符，需要通过把词法序号转换为语法序号，然后得到语法单元的名称。

## 4. 错误恢复

文法错误：

1. 没有定义的符号，直接抛出错误
2. 不符合规范的数字等，按照id处理

语法错误：

1. 尝试从(), []配对同步
2. 尝试错误Exp表达式
3. 尝试从；同步
4. 尝试从{}同步

## 5. 实验感想

bison和flex的组合用起来不是很方便，尤其是对C程序，bison中很多有用的变量在manual中并没有说明，例如yyn；有说明的也往往是一句话。需要通过读源代码来查看其含义

错误恢复的可能性太多，最严谨的做法是遇到错误就报错返回，但往往不是很好用

## 6. 遇到的问题

在mac上没有-lfl库，搜索之后找到了代替方法：1.使用-ll 2.在.l文件最开始加上%option noyywrap, 并且不定义yywrap()函数

makefile测试只测一个文件，在本地修改makefile中test选项，提交时再修改

在确定错误行数的时候一直直接使用的是自己维护的lines，在找错误时以为是规则不够导致的错误，实际上是应该输出error的行号
