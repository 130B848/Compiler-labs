#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"

/*Lab5: Your implementation of lab5*/
E_enventry E_VarEntry(Tr_access acc, Ty_ty ty) {
    E_enventry varEntry = checked_malloc(sizeof(struct E_enventry_));
    varEntry->u.var.access = acc;
    varEntry->kind = E_varEntry;
    varEntry->u.var.ty = ty;
    return varEntry;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result) {
    E_enventry funEntry = checked_malloc(sizeof(struct E_enventry_));
    funEntry->u.fun.level = level;
    funEntry->u.fun.label = label;
    funEntry->kind = E_funEntry;
    funEntry->u.fun.formals = formals;
    funEntry->u.fun.result = result;
    return funEntry;
}

S_table E_base_tenv(void) {
    S_table table = S_empty();
    S_enter(table, S_Symbol("nil"), Ty_Nil());
    S_enter(table, S_Symbol("int"), Ty_Int());
    S_enter(table, S_Symbol("string"), Ty_String());
    S_enter(table, S_Symbol("void"), Ty_Void());
    return table;
}

S_table E_base_venv(void) {
    S_table table = S_empty();
    S_enter(table, S_Symbol("printi"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Void()));
    S_enter(table, S_Symbol("print"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Void()));
    S_enter(table, S_Symbol("flush"), E_FunEntry(Tr_outermost(), Temp_newlabel(), NULL, Ty_Void()));
    S_enter(table, S_Symbol("getchar"), E_FunEntry(Tr_outermost(), Temp_newlabel(), NULL, Ty_String()));
    S_enter(table, S_Symbol("ord"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_Int()));
    S_enter(table, S_Symbol("chr"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_String()));
    S_enter(table, S_Symbol("size"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_String()));
    S_enter(table, S_Symbol("substring"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), Ty_TyList(Ty_Int(), Ty_TyList(Ty_Int(), NULL))), Ty_String()));
    S_enter(table, S_Symbol("concat"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_String(), NULL), Ty_String()));
    S_enter(table, S_Symbol("not"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Int()));
    S_enter(table, S_Symbol("exit"), E_FunEntry(Tr_outermost(), Temp_newlabel(), Ty_TyList(Ty_Int(), NULL), Ty_Void()));
    return table;
}
