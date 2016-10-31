#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/
void SEM_transProg(A_exp exp){
    reset = 0;
    S_table venv = E_base_venv();
    S_table tenv = E_base_tenv();
    transExp(venv, tenv, exp);
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
    switch (v->kind) {
        case A_simpleVar: {
            E_enventry x = S_look(venv, v->u.simple);
            if (x && x->kind == E_varEntry) {
                return expTy(NULL, actual_ty(x->u.var.ty));
            } else {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(NULL, Ty_Int());
            }
        }
        case A_fieldVar: {
            struct expty prev = transVar(venv, tenv, v->u.field.var);
            if (prev.ty->kind != Ty_Record) {
                EM_error(v->pos, "record type variable expected");
            }

            Ty_fieldList fieldList = prev.ty->u.record;
            while (fieldList && fieldList->head->name != v->u.field.sym) {
                fieldList = fieldList->tail;
            }
            if (fieldList) {
                return expTy(NULL, actual_ty(fieldList->head->ty));
            } else {
                EM_error(v->pos, "undefined field %s", S_name(v->u.field.sym));
                return expTy(NULL, NULL);
            }
        }
        case A_subscriptVar: {
            struct expty var = transVar(venv, tenv, v->u.subscript.var);
            if (var.ty->kind != Ty_array) {
                EM_error(v->pos, "array type variable expected");
                return expTy(NULL, NULL);
            }

            struct expty exp = transExp(venv, tenv, v->u.subscript.exp);
            if (exp.ty->kind != Ty_int) {
                EM_error(v->pos, "int type variable expected");
                return expTy(NULL, Ty_Int());
            }

            return expTy(NULL, actual_ty(var.ty->u.array));
        }
        default: {
            return expTy(NULL, NULL);
        }
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {

}

void transDec(S_table venv, S_table tenv, A_dec d) {

}

Ty_ty transTy(S_table tenv, A_ty a) {

}

Ty_ty actual_ty(Ty_ty ty) {
    while (ty && ty->kind == Ty_name) {
        ty = ty->u.name.ty;
    }
    return ty;
}
