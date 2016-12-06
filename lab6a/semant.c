#include <stdio.h>
#include "util.h"
#include "errormsg.h"
#include "symbol.h"
#include "absyn.h"
#include "types.h"
#include "temp.h"
#include "tree.h"
#include "frame.h"
#include "translate.h"
#include "env.h"
#include "semant.h"
#include "printtree.h"

/*Lab5: Your implementation of lab5*/
static A_exp checkFor;

struct expty expTy(Tr_exp exp, Ty_ty ty) {
    struct expty e;
    e.exp = exp;
    e.ty = ty;
    return e;
}

F_fragList SEM_transProg(A_exp exp) {
	struct expty et;
	S_table t = E_base_tenv();
	S_table v = E_base_venv();
	et = transExp(v, t, exp, Tr_outermost());
    Tr_procEntryExit(Tr_outermost(), et.exp, NULL);
	F_fragList resl = Tr_getResult();
	return resl;
}

struct expty transVar(S_table venv, S_table tenv, A_var v, Tr_level level) {
    // EM_error(v->pos, "transVar %d, level = %s", v->kind, Temp_labelstring(level->name));
    switch (v->kind) {
        case A_simpleVar: {
            Tr_exp trans = Tr_nilExp();
            E_enventry x = S_look(venv, v->u.simple);
            if (x && x->kind == E_varEntry) {
                trans = Tr_simpleVar(x->u.var.access, level);
                // EM_error(v->pos, "variable name %s", S_name(v->u.simple));
                return expTy(trans, actual_ty(x->u.var.ty));
            } else {
                EM_error(v->pos, "undefined variable %s", S_name(v->u.simple));
                return expTy(trans, Ty_Int());
            }
        }
		case A_fieldVar: {
            Tr_exp trans = Tr_nilExp();
            struct expty var = transVar(venv, tenv, v->u.field.var, level);
            if (var.ty->kind != Ty_record) {
                EM_error(v->pos, "not a record type");
                return expTy(NULL, NULL);
            }

            Ty_fieldList fieldList = var.ty->u.record;
            int i = 0;
            while (fieldList && fieldList->head->name != v->u.field.sym) {
                fieldList = fieldList->tail;
                i++;
            }
            if (fieldList) {
                trans = Tr_fieldVar(var.exp, i);
                return expTy(trans, actual_ty(fieldList->head->ty));
            } else {
                EM_error(v->pos, "field %s doesn't exist", S_name(v->u.field.sym));
                return expTy(NULL, NULL);
            }
        }
        case A_subscriptVar: {
            Tr_exp trans = Tr_nilExp();
            struct expty var = transVar(venv, tenv, v->u.subscript.var, level);
            if (var.ty->kind != Ty_array) {
                EM_error(v->pos, "array type required");
                return expTy(NULL, Ty_Record(NULL));
            }

            struct expty exp = transExp(venv, tenv, v->u.subscript.exp, level);
            if (exp.ty->kind != Ty_int) {
                EM_error(v->pos, "int type variable expected");
                return expTy(NULL, Ty_Int());
            }

            trans = Tr_subscriptVar(var.exp, exp.exp);
            return expTy(trans, actual_ty(var.ty->u.array));
        }
        default: {
            EM_error(v->pos, "transVar error");
            return expTy(NULL, NULL);
        }
    }
}

struct expty transExp(S_table venv, S_table tenv, A_exp a, Tr_level level) {
    // EM_error(a->pos, "transExp %d, level = %s", a->kind, Temp_labelstring(level->name));
    switch (a->kind) {
        case A_varExp: {
            return transVar(venv, tenv, a->u.var, level);
        }
        case A_nilExp: {
            return expTy(Tr_nilExp(), Ty_Nil());
        }
        case A_intExp: {
            return expTy(Tr_intExp(a->u.intt), Ty_Int());
        }
        case A_stringExp: {
            return expTy(Tr_stringExp(a->u.stringg), Ty_String());
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
            Tr_expList argList = NULL;
            for (;formals && args; formals = formals->tail, args = args->tail) {
                et = transExp(venv, tenv, args->head, level);
                if (!Ty_cmp(formals->head, et.ty)) {
                    EM_error(a->pos, "para type mismatch");
                    return expTy(NULL, Ty_Void());
                }
                argList = Tr_ExpList(et.exp, argList);
            }
            if (formals && !args) {
                EM_error(a->pos, "too few params in function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Void());
            } else if (!formals && args) {
                EM_error(a->pos, "too many params in function %s", S_name(a->u.call.func));
                return expTy(NULL, Ty_Void());
            }

            Tr_exp trans = Tr_callExp(x->u.fun.label, x->u.fun.level, level, argList);
            if (x->u.fun.result) {
                return expTy(trans, actual_ty(x->u.fun.result));
            } else {
                return expTy(trans, Ty_Void());
            }
        }
        case A_opExp: {
            // EM_error(a->pos, "A_opExp oper = %d", a->u.op.oper);
            A_oper oper = a->u.op.oper;
            struct expty left = transExp(venv, tenv, a->u.op.left, level);
            struct expty right = transExp(venv, tenv, a->u.op.right, level);
            Tr_exp trans = Tr_nilExp();

            if (oper == A_plusOp || oper == A_minusOp ||
                oper == A_timesOp || oper == A_divideOp) {
                if (left.ty->kind != Ty_int) {
                    EM_error(a->u.op.left->pos, "integer required");
                }
                if (right.ty->kind != Ty_int) {
                    EM_error(a->u.op.right->pos, "integer required");
                }
                trans = Tr_binOpExp(oper, left.exp, right.exp);
            } else if (oper == A_eqOp || oper == A_neqOp ||
                    oper == A_leOp || oper == A_ltOp ||
                    oper == A_gtOp || oper == A_geOp) {
                if (left.ty->kind == Ty_nil && right.ty->kind == Ty_nil) {
                    EM_error(a->pos, "same type required");
                } else if ((left.ty->kind == Ty_nil && right.ty->kind == Ty_record) ||
                        (left.ty->kind == Ty_record && right.ty->kind == Ty_nil)) {
                    return expTy(trans, Ty_Int());
                } else if (left.ty->kind != right.ty->kind) {
                    EM_error(a->pos, "same type required");
                }
                trans = Tr_relOpExp(oper, left.exp, right.exp);
            }

            return expTy(trans, Ty_Int());
        }
        case A_recordExp: {
            Tr_exp trans = Tr_nilExp();
            Ty_ty record = actual_ty(S_look(tenv, a->u.record.typ));
            if (!record || record->kind != Ty_record) {
                EM_error(a->pos, "undefined type %s", S_name(a->u.record.typ));
                return expTy(trans, Ty_Record(NULL));
            }

            A_efieldList efieldList = a->u.record.fields;
            Ty_fieldList fieldList = record->u.record;

            A_efield efield;
            Ty_field field;
            struct expty et;
            Tr_expList argList = NULL;
            int n = 0;
            for (; efieldList && fieldList;
                    efieldList = efieldList->tail, fieldList = fieldList->tail) {
                efield = efieldList->head;
                field = fieldList->head;
                et = transExp(venv, tenv, efield->exp, level);

                if (!Ty_cmp(field->ty, et.ty)) {
                    EM_error(a->pos, "same type expected in record %s", S_name(a->u.record.typ));
                    return expTy(trans, Ty_Record(NULL));
                }

                argList = Tr_ExpList(et.exp, argList);
                n++;
            }
            trans = Tr_recordExp(n, argList);
            if (efield && !field) {
                EM_error(a->pos, "too many fields in record %s", S_name(a->u.record.typ));
                return expTy(trans, Ty_Record(NULL));
            } else if (!efield && field) {
                EM_error(a->pos, "too many fields in record %s", S_name(a->u.record.typ));
                return expTy(trans, Ty_Record(NULL));
            } else {
                return expTy(trans, record);
            }
        }
        case A_seqExp: {
            Tr_expList argList = NULL;
            Tr_exp trans = Tr_nilExp();
            A_expList seq = a->u.seq;
            struct expty tmp;
            if (!seq) {
                return expTy(trans, Ty_Void());
            }

            for (; seq; seq = seq->tail) {
                tmp = transExp(venv, tenv, seq->head, level);
                argList = Tr_ExpList(tmp.exp, argList);
            }
            trans = Tr_seqExp(argList);
            return expTy(trans, tmp.ty);
        }
        case A_assignExp: {
            Tr_exp trans = Tr_nilExp();
            struct expty var = transVar(venv, tenv, a->u.assign.var, level);
            struct expty exp = transExp(venv, tenv, a->u.assign.exp, level);
            if (!var.ty || !exp.ty) {
                return expTy(trans, Ty_Void());
            }
            if (!Ty_cmp(var.ty, exp.ty)) {
                EM_error(a->pos, "unmatched assign exp");
            }
            if (a->u.assign.var->kind == A_simpleVar && checkFor &&
                    !strcmp(a->u.assign.var->u.simple, checkFor->u.forr.var)) {
                EM_error(a->pos, "loop variable can't be assigned");
            }
            trans = Tr_assignExp(var.exp, exp.exp);
            return expTy(trans, Ty_Void());
        }
        case A_ifExp: {
            struct expty test = transExp(venv, tenv, a->u.iff.test, level);
            // EM_error(a->pos, "1111111111111111111111111111111111A_ifExp test's kind is %d", a->u.iff.test->kind);
            if (test.ty->kind != Ty_int) {
                EM_error(a->pos, "if must judge an integer");
            }

            struct expty elsee;
            struct expty then = transExp(venv, tenv, a->u.iff.then, level);
            if (a->u.iff.elsee) {
                elsee = transExp(venv, tenv, a->u.iff.elsee, level);
                if (!Ty_cmp(then.ty, elsee.ty)) {
                    EM_error(a->pos, "then exp and else exp type mismatch");
                }
                return expTy(Tr_ifExp(test.exp, then.exp, elsee.exp), then.ty);
                //return expTy(Tr_nilExp(), then.ty);
            } else {
                // EM_error(a->pos, "4444444444444444444444444444444444no else get through body");
                if (then.ty->kind != Ty_void) {
                    EM_error(a->pos, "if-then exp's body must produce no value");
                }
                return expTy(Tr_ifExp(test.exp, then.exp, NULL), Ty_Void());
            }
        }
        case A_whileExp: {
            struct expty test = transExp(venv, tenv, a->u.whilee.test, level);
            if (test.ty->kind != Ty_int) {
                EM_error(a->pos, "while must judge an integer");
            }

            Tr_exp done = Tr_doneExp();
            struct expty body = transExp(venv, tenv, a->u.whilee.body, level);
            if (body.ty->kind == Ty_int) {
                EM_error(a->pos, "while body must produce no value");
            }
            return expTy(Tr_whileExp(test.exp, body.exp, done), Ty_Void());
        }
        case A_forExp: {
            struct expty lo = transExp(venv, tenv, a->u.forr.lo, level);
            struct expty hi = transExp(venv, tenv, a->u.forr.hi, level);
            struct expty body;

            if (lo.ty->kind != Ty_int || hi.ty->kind != Ty_int) {
                EM_error(a->pos, "for exp's range type is not integer");
            }

            S_beginScope(venv);
            Tr_access acc = Tr_allocLocal(level, a->u.forr.escape);
            transDec(venv, tenv, A_VarDec(a->pos, a->u.forr.var, S_Symbol("int"), a->u.forr.lo), level);
            checkFor = a;
            // EM_error(a->pos, "222222222222222222222forExp get through body %d", a->u.forr.body->kind);
            body = transExp(venv, tenv, a->u.forr.body, level);
            checkFor = NULL;
            S_endScope(venv);
            return expTy(Tr_forExp(lo.exp, hi.exp, body.exp, acc), Ty_Void());
        }
        case A_breakExp: {
            return expTy(Tr_breakExp(), Ty_Void());
        }
        case A_letExp: {
            // EM_error(a->pos, "letExp get in");
            struct expty ret;
            Tr_expList argList = NULL;

            S_beginScope(venv);
            S_beginScope(tenv);

            A_decList decs = a->u.let.decs;
            for (; decs; decs = decs->tail) {
                Tr_exp tmp = transDec(venv, tenv, decs->head, level);
                argList = Tr_ExpList(tmp, argList);
            }

            // EM_error(a->pos, "letExp get through here 33333333333333333333333");
            ret = transExp(venv, tenv, a->u.let.body, level);
            argList = Tr_ExpList(ret.exp, argList);

            S_endScope(tenv);
            S_endScope(venv);
            // EM_error(a->pos, "letExp get out");
            return expTy(Tr_seqExp(argList), ret.ty);
        }
        case A_arrayExp: {
            Tr_exp trans = Tr_nilExp();
            Ty_ty actual = actual_ty(S_look(tenv, a->u.array.typ));
            if (!actual) {
                EM_error(a->pos, "undefined array %s", S_name(a->u.array.typ));
                return expTy(trans, Ty_Array(NULL));
            }
            if (actual->kind != Ty_array) {
                EM_error(a->pos, "array type expected");
                return expTy(trans, Ty_Array(NULL));
            }

            struct expty size = transExp(venv, tenv, a->u.array.size, level);
            struct expty init = transExp(venv, tenv, a->u.array.init, level);
            if (size.ty->kind != Ty_int) {
                EM_error(a->pos, "integer type expected");
            } else if (!Ty_cmp(init.ty, actual->u.array)) {
                EM_error(a->u.array.init->pos, "type mismatch");
            }
            trans = Tr_arrayExp(size.exp, init.exp);

            return expTy(trans, actual);
        }
        default: {
            return expTy(NULL, NULL);
        }
    }
}

Tr_exp transDec(S_table venv, S_table tenv, A_dec d, Tr_level level) {
    // EM_error(d->pos, "transDec %d, level = %s", d->kind, Temp_labelstring(level->name));
    switch (d->kind) {
        case A_functionDec: {
            A_fundecList fdl = d->u.function;
            Ty_ty resultTy;
            Ty_tyList formalTys;
            U_boolList bl = NULL;
            for (; fdl; fdl = fdl->tail) {
                resultTy = Ty_Void();
                //if (S_look(venv, fdl->head->name)) {
                //    EM_error(d->pos, "two functions have the same name");
                //    return;
                //}
                if (fdl->head->result) {
                    // EM_error(0, "name %s result %s", S_name(fdl->head->name), S_name(fdl->head->result));
                    resultTy = S_look(tenv, fdl->head->result);
                    if (!resultTy) {
                        EM_error(fdl->head->pos, "undefined type for return");
                    }
                }
                formalTys = makeFormalTyList(tenv, fdl->head->params);
                Temp_label funcLabel = Temp_newlabel();
                A_fieldList params = fdl->head->params;
                for (; params; params = params->tail) {
                    bl = U_BoolList(TRUE, bl);
                }
                Tr_level l = Tr_newLevel(level, funcLabel, bl);
                S_enter(venv, fdl->head->name, E_FunEntry(l, funcLabel, formalTys, resultTy));
            }

            A_fieldList fieldList;
            Ty_tyList tyList;
            struct expty body;
            E_enventry func;
            for (fdl = d->u.function; fdl; fdl = fdl->tail) {
                E_enventry funEntry = S_look(venv, fdl->head->name);
                S_beginScope(venv);
                formalTys = makeFormalTyList(tenv, fdl->head->params);
                fieldList = fdl->head->params;
                tyList = formalTys;
                Tr_accessList acls = Tr_formals(funEntry->u.fun.level);
                for (; fieldList && tyList && acls; fieldList = fieldList->tail, tyList = tyList->tail, acls = acls->tail) {
                    S_enter(venv, fieldList->head->name, E_VarEntry(acls->head, tyList->head));
                }

                body = transExp(venv, tenv, fdl->head->body, funEntry->u.fun.level);
                // EM_error(0, "func %s body %d", S_name(fdl->head->name), body.ty->kind);
                func = S_look(venv, fdl->head->name);
                if (!Ty_cmp(body.ty, func->u.fun.result)) {
                    // EM_error(0, "tt->kind %d, ee->kind %d", body.ty->kind, func->u.fun.result->kind);
                    if (func->u.fun.result->kind == Ty_void)
                        EM_error(fdl->head->pos, "procedure returns value");
                    else
                        EM_error(fdl->head->pos, "invalid return type in function %s", S_name(fdl->head->name));
                }
                Tr_procEntryExit(funEntry->u.fun.level, body.exp, acls);
                S_endScope(venv);
            }
            return Tr_nilExp();
        }
        case A_varDec: {
            struct expty init = transExp(venv, tenv, d->u.var.init, level);
            Tr_access acc = Tr_allocLocal(level, d->u.var.escape);
            if (d->u.var.typ == NULL) {
                //S_enter(venv, d->u.var.var, E_VarEntry(init.ty));
                if (init.ty->kind == Ty_nil || init.ty->kind == Ty_void) {
                    EM_error(d->pos, "init should not be nil without type specified");
                    S_enter(venv, d->u.var.var, E_VarEntry(acc, init.ty));
                } else {
                    S_enter(venv, d->u.var.var, E_VarEntry(acc, init.ty));
                }
            } else {
                Ty_ty resultTy = S_look(tenv, d->u.var.typ);
                if (!resultTy) {
                    EM_error(d->pos, "undefined type %s", S_name(d->u.var.typ));
                } else {
                    if (!Ty_cmp(resultTy, init.ty)) {
                        //EM_error(d->pos, "same type expected in %s", S_name(d->u.var.typ));
                        EM_error(d->pos, "type mismatch");
                        S_enter(venv, d->u.var.var, E_VarEntry(acc, resultTy));
                    } else {
                        S_enter(venv, d->u.var.var, E_VarEntry(acc, resultTy));
                    }
                }
            }
            return Tr_assignExp(Tr_simpleVar(acc, level), init.exp);
        }
        case A_typeDec: {
            A_nametyList nl = d->u.type;
            for (; nl; nl = nl->tail) {
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
                if (!nl->tail && (actual_ty(resultTy)->kind == Ty_name)) {
                    EM_error(d->pos, "no actual type defined");
                }
                Ty_ty namety = S_look(tenv, nl->head->name);
                namety->u.name.ty = actual_ty(resultTy);
            }
            if (checkCycle) {
                EM_error(d->pos, "illegal type cycle");
            }
            return Tr_nilExp();
        }
        default: {
            break;
        }
    }
}

Ty_ty transTy(S_table tenv, A_ty a) {
    // EM_error(a->pos, "transTy %d", a->kind);
    switch (a->kind) {
        case A_nameTy: {
            if (S_Symbol("int") == a->u.name) {
                return Ty_Int();
            }
            if (S_Symbol("string") == a->u.name) {
                return Ty_String();
            }
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
