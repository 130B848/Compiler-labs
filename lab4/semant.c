#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "env.h"
#include "semant.h"

/*Lab4: Your implementation of lab4*/
static A_exp checkFor;

struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

void SEM_transProg(A_exp exp){
    checkFor = NULL;
    S_table venv = E_base_venv();
    S_table tenv = E_base_tenv();
    transExp(venv, tenv, exp);
}

struct expty transVar(S_table venv, S_table tenv, A_var v) {
    //EM_error(v->pos, "transVar %d", v->kind);
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
            struct expty var = transVar(venv, tenv, v->u.field.var);
            if (var.ty->kind != Ty_record) {
                EM_error(v->pos, "not a record type");
                return expTy(NULL, NULL);
            }

            Ty_fieldList fieldList = var.ty->u.record;
            while (fieldList && fieldList->head->name != v->u.field.sym) {
                fieldList = fieldList->tail;
            }
            if (fieldList) {
                return expTy(NULL, actual_ty(fieldList->head->ty));
            } else {
                EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
                return expTy(NULL, NULL);
            }
        }
        case A_subscriptVar: {
            struct expty var = transVar(venv, tenv, v->u.subscript.var);
            if (var.ty->kind != Ty_array) {
                EM_error(v->pos, "array type required");
                return expTy(NULL, Ty_Record(NULL));
            }

            struct expty exp = transExp(venv, tenv, v->u.subscript.exp);
            if (exp.ty->kind != Ty_int) {
                EM_error(v->pos, "int type variable expected");
                return expTy(NULL, Ty_Int());
            }

            return expTy(NULL, actual_ty(var.ty->u.array));
        }
        default: {
            EM_error(v->pos, "transVar error");
            return expTy(NULL, NULL);
        }
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a) {
    //EM_error(a->pos, "transExp %d", a->kind);
    switch (a->kind) {
        case A_varExp: {
            return transVar(venv, tenv, a->u.var);
        }
        case A_nilExp: {
            return expTy(NULL, Ty_Nil());
        }
        case A_intExp: {
            return expTy(NULL, Ty_Int());
        }
        case A_stringExp: {
            return expTy(NULL, Ty_String());
        }
        case A_callExp: {
            E_enventry x = S_look(venv, a->u.call.func);
            if (!x || x->kind != E_funEntry) {
                EM_error(a->pos, "undefined function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Int());
            }

            Ty_tyList formals = x->u.fun.formals;
            A_expList args = a->u.call.args;
            struct expty et;
            for (;formals && args; formals = formals->tail, args = args->tail) {
                et = transExp(venv, tenv, args->head);

                if (!Ty_cmp(formals->head, et.ty)) {
                    //EM_error(a->pos, "same types expected in function %s", S_name(a->u.call.func));
                    EM_error(a->pos, "para type mismatch");
                    return expTy(NULL, Ty_Void());
                }
            }
            if (formals && !args) {
                EM_error(a->pos, "too few params in function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Void());
            } else if (!formals && args) {
                EM_error(a->pos, "too many params in function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Void());
            }

            if (x->u.fun.result) {
                return expTy(NULL, actual_ty(x->u.fun.result));
            } else {
                return expTy(NULL, Ty_Void());
            }
        }
        case A_opExp: {
            A_oper oper = a->u.op.oper;
            struct expty left = transExp(venv, tenv, a->u.op.left);
            struct expty right = transExp(venv, tenv, a->u.op.right);

            if (oper == A_plusOp || oper == A_minusOp ||
                oper == A_timesOp || oper == A_divideOp) {
                if (left.ty->kind != Ty_int) {
                    EM_error(a->u.op.left->pos, "integer required");
                }
                if (right.ty->kind != Ty_int) {
                    EM_error(a->u.op.right->pos, "integer required");
                }
            } else if (oper == A_eqOp || oper == A_neqOp ||
                    oper == A_leOp || oper == A_ltOp ||
                    oper == A_gtOp || oper == A_geOp) {
                if (left.ty->kind == Ty_nil && right.ty->kind == Ty_nil) {
                    EM_error(a->pos, "same type required");
                } else if ((left.ty->kind == Ty_nil && right.ty->kind == Ty_record) ||
                        (left.ty->kind == Ty_record && right.ty->kind == Ty_nil)) {
                    return expTy(NULL, Ty_Int());
                } else if (left.ty->kind != right.ty->kind) {
                    EM_error(a->pos, "same type required");
                }
            }
            return expTy(NULL, Ty_Int());
        }
        case A_recordExp: {
            Ty_ty record = actual_ty(S_look(tenv, a->u.record.typ));
            if (!record || record->kind != Ty_record) {
                EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Record(NULL));
            }

            A_efieldList efieldList = a->u.record.fields;
            Ty_fieldList fieldList = record->u.record;

            A_efield efield;
            Ty_field field;
            struct expty et;
            for (; efieldList && fieldList;
                    efieldList = efieldList->tail, fieldList = fieldList->tail) {
                efield = efieldList->head;
                field = fieldList->head;
                et = transExp(venv, tenv, efield->exp);

                if (!Ty_cmp(field->ty, et.ty)) {
                    EM_error(a->pos, "same type expected in record %s", S_name(a->u.record.typ));
                    return expTy(NULL, Ty_Record(NULL));
                }
            }
            if (efield && !field) {
                EM_error(a->pos, "too many fields in record %s", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Record(NULL));
            } else if (!efield && field) {
                EM_error(a->pos, "too many fields in record %s", S_name(a->u.record.typ));
                return expTy(NULL, Ty_Record(NULL));
            } else {
                return expTy(NULL, record);
            }
        }
        case A_seqExp: {
            A_expList seq = a->u.seq;
            if (!seq) {
                return expTy(NULL, Ty_Void());
            }

            for (; seq->tail; seq = seq->tail) {
                transExp(venv, tenv, seq->head);
            }
            return transExp(venv, tenv, seq->head);
        }
        case A_assignExp: {
            struct expty var = transVar(venv, tenv, a->u.assign.var);
            struct expty exp = transExp(venv, tenv, a->u.assign.exp);
            if (!var.ty || !exp.ty) {
                return expTy(NULL, Ty_Void());
            }
            if (!Ty_cmp(var.ty, exp.ty)) {
                EM_error(a->pos, "unmatched assign exp");
            }
            if (a->u.assign.var->kind == A_simpleVar && checkFor && 
                    !strcmp(a->u.assign.var->u.simple, checkFor->u.forr.var)) {
                EM_error(a->pos, "loop variable can't be assigned");
            }
            return expTy(NULL, Ty_Void());
        }
        case A_ifExp: {
            struct expty test = transExp(venv, tenv, a->u.iff.test);
            if (test.ty->kind != Ty_int) {
                EM_error(a->pos, "if must judge an integer");
            }

            struct expty then = transExp(venv, tenv, a->u.iff.then);
            if (a->u.iff.elsee) {
                struct expty elsee = transExp(venv, tenv, a->u.iff.elsee);
                if (!Ty_cmp(then.ty, elsee.ty)) {
                    EM_error(a->pos, "then exp and else exp type mismatch");
                }
            } else {
                if (then.ty->kind != Ty_void) {
                    EM_error(a->pos, "if-then exp's body must produce no value");
                }
            }

            return expTy(NULL, then.ty);
        }
        case A_whileExp: {
            struct expty test = transExp(venv, tenv, a->u.whilee.test);
            if (test.ty->kind != Ty_int) {
                EM_error(a->pos, "while must judge an integer");
            }

            struct expty body = transExp(venv, tenv, a->u.whilee.body);
            if (body.ty->kind == Ty_int) {
                EM_error(a->pos, "while body must produce no value");
            }
            return expTy(NULL, Ty_Void());
        }
        case A_forExp: {
            struct expty lo = transExp(venv, tenv, a->u.forr.lo);
            struct expty hi = transExp(venv, tenv, a->u.forr.hi);
            struct expty body;

            if (lo.ty->kind != Ty_int || hi.ty->kind != Ty_int) {
                EM_error(a->pos, "for exp's range type is not integer");
            }

            S_beginScope(venv);
            transDec(venv, tenv, A_VarDec(a->pos, a->u.forr.var, S_Symbol("int"), a->u.forr.lo));
            checkFor = a;
            body = transExp(venv, tenv, a->u.forr.body);
            checkFor = NULL;
            S_endScope(venv);
            return expTy(NULL, Ty_Void());
        }
        case A_breakExp: {
            return expTy(NULL, Ty_Void());
        }
        case A_letExp: {
            struct expty ret;

            S_beginScope(venv);
            S_beginScope(tenv);

            A_decList decs = a->u.let.decs;
            for (; decs; decs = decs->tail) {
                transDec(venv, tenv, decs->head);
            }
            ret = transExp(venv, tenv, a->u.let.body);

            S_endScope(tenv);
            S_endScope(venv);
            return ret;
        }
        case A_arrayExp: {
            Ty_ty actual = actual_ty(S_look(tenv, a->u.array.typ));
            if (!actual) {
                EM_error(a->pos, "undefined array %s", S_name(a->u.array.typ));
                return expTy(NULL, Ty_Array(NULL));
            }
            if (actual->kind != Ty_array) {
                EM_error(a->pos, "array type expected");
                return expTy(NULL, Ty_Array(NULL));
            }

            struct expty size = transExp(venv, tenv, a->u.array.size);
            struct expty init = transExp(venv, tenv, a->u.array.init);
            if (size.ty->kind != Ty_int) {
                EM_error(a->pos, "integer type expected");
            } else if (!Ty_cmp(init.ty, actual->u.array)) {
                EM_error(a->u.array.init->pos, "type mismatch");
            }

            return expTy(NULL, actual);
        }
        default: {
            return expTy(NULL, NULL);
        }
    }
}

void transDec(S_table venv, S_table tenv, A_dec d) {
    //EM_error(d->pos, "transDec %d", d->kind);
    switch (d->kind) {
        case A_functionDec: {
            A_fundecList fdl = d->u.function;
            Ty_ty resultTy = Ty_Void();
            Ty_tyList formalTys;
            for (; fdl; fdl = fdl->tail) {
                if (S_look(venv, fdl->head->name)) {
                    EM_error(d->pos, "two functions have the same name");
                    return;
                }
                if (fdl->head->result) {
                    resultTy = S_look(tenv, fdl->head->result);
                    if (!resultTy) {
                        EM_error(fdl->head->pos, "undefined type for return");
                    }
                }
                formalTys = makeFormalTyList(tenv, fdl->head->params);
                S_enter(venv, fdl->head->name, E_FunEntry(formalTys, resultTy));
            }
            
            A_fieldList fieldList;
            Ty_tyList tyList;
            struct expty body;
            E_enventry func;
            for (fdl = d->u.function; fdl; fdl = fdl->tail) {
                S_beginScope(venv);
                formalTys = makeFormalTyList(tenv, fdl->head->params);
                fieldList = fdl->head->params;
                tyList = formalTys;
                for (; fieldList && tyList; fieldList = fieldList->tail, tyList = tyList->tail) {
                    S_enter(venv, fieldList->head->name, E_VarEntry(tyList->head));
                }

                body = transExp(venv, tenv, fdl->head->body);
                func = S_look(venv, fdl->head->name);
                if (!Ty_cmp(body.ty, func->u.fun.result)) {
                    if (func->u.fun.result->kind == Ty_void)
                        EM_error(fdl->head->pos, "procedure returns value");
                    else
                        EM_error(fdl->head->pos, "invalid return type in function %s", S_name(fdl->head->name));
                }
                S_endScope(venv);
            }
            break;
        }
        case A_varDec: {
            struct expty init = transExp(venv, tenv, d->u.var.init);
            if (d->u.var.typ == NULL) {
                //S_enter(venv, d->u.var.var, E_VarEntry(init.ty));
                if (init.ty->kind == Ty_nil || init.ty->kind == Ty_void) {
                    EM_error(d->pos, "init should not be nil without type specified");
                    S_enter(venv, d->u.var.var, E_VarEntry(init.ty));
                } else {
                    S_enter(venv, d->u.var.var, E_VarEntry(init.ty));
                }
            } else {
                Ty_ty resultTy = S_look(tenv, d->u.var.typ);
                if (!resultTy) {
                    EM_error(d->pos, "undefined type %s", S_name(d->u.var.typ));
                } else {
                    if (!Ty_cmp(resultTy, init.ty)) {
                        //EM_error(d->pos, "same type expected in %s", S_name(d->u.var.typ));
                        EM_error(d->pos, "type mismatch");
                        S_enter(venv, d->u.var.var, E_VarEntry(resultTy));
                    } else {
                        S_enter(venv, d->u.var.var, E_VarEntry(resultTy));
                    }
                }
            }
            break;
        }
        case A_typeDec: {
            A_nametyList nl = d->u.type;
            for (; nl; nl = nl->tail) {
                if (S_look(tenv, nl->head->name)) {
                    EM_error(d->pos, "two types have the same name");
                    return;
                }
                S_enter(tenv, nl->head->name, Ty_Name(nl->head->name, NULL));
            }
            
            nl = d->u.type;
            Ty_ty resultTy;
            int checkCycle = 1;
            for (; nl; nl = nl->tail) {
                resultTy = transTy(tenv, nl->head->ty);
                if (checkCycle && (resultTy->kind != Ty_name)) {
                    checkCycle = 0;
                }
                if (!nl->tail && (resultTy->kind == Ty_name)) {
                    EM_error(d->pos, "no actual type defined");
                }
                Ty_ty namety = S_look(tenv, nl->head->name);
                namety->u.name.ty = resultTy;
            }
            if (checkCycle) {
                EM_error(d->pos, "illegal type cycle");
            }
            break;
        }
        default: {
            break;
        }
    }
}

Ty_ty transTy(S_table tenv, A_ty a) {
    switch (a->kind) {
        case A_nameTy: {
            return Ty_Name(a->u.name, S_look(tenv, a->u.name));
        }
        case A_recordTy: {
            Ty_fieldList tfl = NULL;
            A_fieldList afl = a->u.record;

            for (; afl; afl = afl->tail) {
                S_symbol name = afl->head->name;
                Ty_ty ty = S_look(tenv, afl->head->typ);
                if (!ty) {
                    EM_error(afl->head->pos, "undefined type %s", S_name(afl->head->typ));
                }
                tfl = Ty_FieldList(Ty_Field(name, ty), tfl);
            }

            Ty_fieldList tmp = NULL;
			while (tfl){
				tmp = Ty_FieldList(tfl->head, tmp);
				tfl = tfl->tail;
			}

			return Ty_Record(tmp);
        }
		case A_arrayTy: {
			return Ty_Array(S_look(tenv, a->u.array));
		}
		default: {
			return NULL;
		}
    }
}

Ty_ty actual_ty(Ty_ty ty) {
    while (ty && ty->kind == Ty_name) {
        //EM_error(0, "ty->kind = %d", ty->kind);
        ty = ty->u.name.ty;
    }
    return ty;
}

Ty_tyList makeFormalTyList(S_table tenv, A_fieldList f){
	Ty_tyList head = NULL;
	Ty_tyList tail = NULL;

	for (; f; f = f->tail) {
		A_field field = f->head;
		Ty_ty ty = S_look(tenv, field->typ);
		if(tail){
			tail->tail = Ty_TyList(ty, NULL);
			tail = tail->tail;
		} else {
			tail = Ty_TyList(ty, NULL);
			head = tail;
		}
	}

	return head;
}

bool Ty_cmp(Ty_ty tt, Ty_ty ee) {
    Ty_ty t = actual_ty(tt);
    Ty_ty e = actual_ty(ee);
    int tk = t->kind;
    int ek = e->kind;
    //EM_error(0, "tk = %d, ek = %d", tk, ek);

    return (((tk == Ty_record || tk == Ty_array) && t == e) ||
            (tk == Ty_record && ek == Ty_nil) ||
            (ek == Ty_record && tk == Ty_nil) ||
            (tk != Ty_record && tk != Ty_array && tk == ek));
}
