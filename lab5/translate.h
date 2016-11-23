#ifndef TRANSLATE_H
#define TRANSLATE_H
#include "temp.h"
#include "frame.h"
/* Lab5: your code below */
typedef struct Tr_expList_ *Tr_expList;

typedef struct Tr_exp_ *Tr_exp;

typedef struct Tr_access_ *Tr_access;

typedef struct Tr_accessList_ *Tr_accessList;

struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;
};

typedef struct Tr_level_ *Tr_level;

struct Tr_level_ {
	//Lab5: your code here
    Tr_level parent;
    F_frame frame;
    Temp_label name;
};

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail);

Tr_level Tr_outermost(void);

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals);

Tr_accessList Tr_formals(Tr_level level);

Tr_access Tr_allocLocal(Tr_level level, bool escape);

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail);

/* Interface between translate and semantic */
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level level);
Tr_exp Tr_fieldVar(Tr_exp base, int offset);
Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp index);

Tr_exp Tr_nilExp(void);
Tr_exp Tr_intExp(int i);
Tr_exp Tr_stringExp(string str);
Tr_exp Tr_callExp(Temp_label label, Tr_level funcLevel, Tr_level level, Tr_expList expList);
Tr_exp Tr_binOpExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_relOpExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_recordExp(int num, Tr_expList fields);
Tr_exp Tr_seqExp(Tr_expList expList);
Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Tr_exp done);
Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Tr_access acc);
Tr_exp Tr_breakExp();
Tr_exp Tr_letExp(Tr_expList decs, Tr_exp body);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);
Tr_exp Tr_doneExp();

Tr_exp Tr_StaticLink(Tr_level now, Tr_level def);

Tr_exp Tr_functionDec(Tr_expList function);
Tr_exp Tr_varDec(Tr_exp init, Tr_access acc);
Tr_exp Tr_typeDec(Tr_expList type);

void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals);

F_fragList Tr_getResult(void);

#endif
