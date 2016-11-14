#ifndef SEMANT_H
#define SEMANT_H

#include "env.h"

//typedef void *Tr_exp;

struct expty {
    Tr_exp exp;
    Ty_ty ty;
};

struct expty expTy(Tr_exp exp, Ty_ty ty);

F_fragList   SEM_transProg(A_exp exp);
struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level level);
struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level level);
void         transDec(S_table venv, S_table tenv, A_dec d, Tr_level level);
        Ty_ty transTy(              S_table tenv, A_ty a);

Ty_ty actual_ty(Ty_ty ty);
Ty_tyList makeFormalTyList(S_table tenv, A_fieldList f);

bool Ty_cmp(Ty_ty tt, Ty_ty ee);

#endif
