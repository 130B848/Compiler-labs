#include <stdio.h>
#include "util.h"
#include "table.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "tree.h"
#include "printtree.h"
#include "frame.h"
#include "translate.h"

struct Tr_access_ {
	//Lab5: your code here
   Tr_level level;
   F_access access;
};


struct Tr_accessList_ {
	Tr_access head;
	Tr_accessList tail;	
};

struct Tr_level_ {
	//Lab5: your code here
    Tr_level parent;
    F_frame frame;
    Temp_label name;
};

struct Cx {
    patchList trues;
    patchList falses;
    T_stm stm;
};

struct Tr_exp_ {
	//Lab5: your code here
    enum {
        Tr_ex, Tr_nx, Tr_cx
    } kind;
    union {
        T_exp ex;
        T_stm nx;
        struct Cx cx;
    } u;
};

struct Tr_expList_ {
    Tr_exp head;
    Tr_expList tail;
}

static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

typedef struct patchList_ *patchList;
struct patchList_ {
    Temp_label *head;
    patchList tail;
};
static patchList PatchList(Temp_label * head, patchList tail);

static T_exp unEx(Tr_exp);
static T_stm unNx(Tr_exp);
static struct Cx unCx(Tr_exp);

static void doPatch(patchList tList, Temp_label label);
static patchList joinPatch(patchList first, patchList second);

/* Implementations */
Tr_access Tr_Access(Tr_level level, F_access facc) {
    Tr_access acc = checked_malloc(sizeof(Tr_access_));
    acc->level = level;
    acc->acess = facc;
    return acc;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList al = checked_malloc(sizeof(Tr_accessList_));
    al->head = head;
    al->tail = tail;
    return al;
}

Tr_level Tr_outermost(void) {
    static Tr_level outer = NULL;
    if (!outer) {
        outer = Tr_newLevel(NULL, Temp_newlabel(), NULL);
    }
    return outer;
}

Tr_level Tr_newLevel(Tr_level parent, Temp_label name, U_boolList formals) {
    Tr_level l = checked_malloc(sizeof(Tr_level_));
    l->>parent = parent;
    l->name = name;
    l->frame = F_newFrame(name, U_BoolList(TRUE, formals));
    
    return l;
}

Tr_accessList Tr_formals(Tr_level l) {
    l->formals = NULL;
    F_accessList fal = F_formals(l->frame)->tail; /* jump over first static link */
    for (; fal; fal = fal->tail) {
        l->formals = Tr_AccessList(Tr_Access(level, fal->head), l->formals);
    }

    return level->formals;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    Tr_access acc = checked_malloc(sizeof(Tr_access_));
    acc->level = level;
    acc->access = F_allocLocal(level->frame, escape);
    return acc;
}


Tr_exp Tr_nilExp(void) {
    static Temp_temp tmp = NULL;
    if (!tmp) {
        tmp = Temp_newtemp();
        T_stm alloc = T_Move(T_Temp(tmp), 
                T_externalCall(String("initRecord"), T_ExpList(T_Const(0), NULL)));
        return Tr_Ex(T_Eseq(alloc, T_Temp(tmp)));
    }
    return Tr_Ex(T_Temp(tmp));
}

Tr_exp Tr_intExp(int i) {
    return Tr_Ex(T_Const(i));
}

Tr_exp Tr_stringExp(string str) {
    static F_fragList strFragList = NULL;
    Temp_label sl = Temp_newlabel();
    F_frag frag = F_StringFrag(sl, str);
    strFragList = F_FragList(frag, strFragList);
    return Tr_Ex(T_Name(sl));
}

Tr_exp Tr_callExp(Temp_label label, Tr_level funcLevel, Tr_level level, Tr_expList expList) {

}

Tr_exp Tr_opExp(A_oper oper, Tr_exp left, Tr_exp right);
Tr_exp Tr_recordExp(int num, Tr_expList fields);
Tr_exp Tr_seqExp(Tr_expList expList);
Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right);
Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee);
Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body);
Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body);
Tr_exp Tr_breadExp();
Tr_exp Tr_letExp(Tr_expList decs, Tr_exp body);
Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init);

Tr_exp Tr_functionDec(Tr_expList function);
Tr_exp Tr_varDec(Tr_exp init);
Tr_exp Tr_typeDec(Tr_expList type);

static void doPatch(patchList tList, Temp_label label) {
    for (; tList; tList = tList->tail) {
        *(tList->head) = label;
    }
}

static patchList joinPatch(patchList first, patchList second) {
    if (!first) {
        return second;
    }
    for (; first->tail; first = first->tail) { // go to end of list
        first->tail = second;
    }
    return first;
}


