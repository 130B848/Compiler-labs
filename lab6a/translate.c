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

typedef struct patchList_ *patchList;
struct patchList_ {
    Temp_label *head;
    patchList tail;
};
static patchList PatchList(Temp_label *head, patchList tail) {
    patchList pl = checked_malloc(sizeof(struct patchList_));
    pl->head = head;
    pl->tail = tail;
    return pl;
}

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
};

static Tr_exp Tr_Ex(T_exp ex);
static Tr_exp Tr_Nx(T_stm nx);
static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm);

static T_exp unEx(Tr_exp e);
static T_stm unNx(Tr_exp e);
static struct Cx unCx(Tr_exp e);

static void doPatch(patchList tList, Temp_label label);
static patchList joinPatch(patchList first, patchList second);

/* Implementations */
static Tr_exp Tr_Ex(T_exp ex) {
    Tr_exp e = checked_malloc(sizeof(struct Tr_exp_));
    e->kind = Tr_ex;
    e->u.ex = ex;
    return e;
}

static Tr_exp Tr_Nx(T_stm nx) {
    Tr_exp e = checked_malloc(sizeof(struct Tr_exp_));
    e->kind = Tr_nx;
    e->u.nx = nx;
    return e;
}

static Tr_exp Tr_Cx(patchList trues, patchList falses, T_stm stm) {
    Tr_exp e = checked_malloc(sizeof(struct Tr_exp_));
    e->kind = Tr_cx;
    e->u.cx.trues = trues;
    e->u.cx.falses = falses;
    e->u.cx.stm = stm;
    return e;
}

static T_exp unEx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex:
            return e->u.ex;
        case Tr_nx:
            return T_Eseq(e->u.nx, T_Const(0));
        case Tr_cx: {
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Eseq(T_Move(T_Temp(r), T_Const(1)),
                    T_Eseq(e->u.cx.stm,
                        T_Eseq(T_Label(f),
                            T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                                    T_Eseq(T_Label(t), T_Temp(r))))));
        }
        default:
            return NULL;
    }
}

static T_stm unNx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex:
            return T_Exp(e->u.ex);
        case Tr_nx:
            return e->u.nx;
        case Tr_cx:{
            Temp_temp r = Temp_newtemp();
            Temp_label t = Temp_newlabel(), f = Temp_newlabel();
            doPatch(e->u.cx.trues, t);
            doPatch(e->u.cx.falses, f);
            return T_Exp(T_Eseq(T_Move(T_Temp(r), T_Const(1)),
                    T_Eseq(e->u.cx.stm,
                        T_Eseq(T_Label(f),
                            T_Eseq(T_Move(T_Temp(r), T_Const(0)),
                                    T_Eseq(T_Label(t), T_Temp(r)))))));
        }
        default:
            return NULL;
    }
}

static struct Cx unCx(Tr_exp e) {
    switch (e->kind) {
        case Tr_ex:{
            struct Cx cx;
            cx.stm = T_Cjump(T_eq, e->u.ex, T_Const(0), NULL, NULL);
            cx.trues = PatchList(&(cx.stm->u.CJUMP.true), NULL);
            cx.falses = PatchList(&(cx.stm->u.CJUMP.false), NULL);
            return cx;
        }
        case Tr_nx:
            //cx.stm = e->u.nx->u.SEQ
            assert(0);
        case Tr_cx:
            return e->u.cx;
        default:
            assert(0);
    }
}

Tr_access Tr_Access(Tr_level level, F_access facc) {
    Tr_access acc = checked_malloc(sizeof(struct Tr_access_));
    acc->level = level;
    acc->access = facc;
    return acc;
}

Tr_accessList Tr_AccessList(Tr_access head, Tr_accessList tail) {
    Tr_accessList al = checked_malloc(sizeof(struct Tr_accessList_));
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
    Tr_level l = checked_malloc(sizeof(struct Tr_level_));
    l->parent = parent;
    l->name = name;
    l->frame = F_newFrame(name, U_BoolList(TRUE, formals));

    return l;
}

Tr_accessList Tr_formals(Tr_level l) {
    Tr_accessList formals = NULL;
    F_accessList fal = F_formals(l->frame)->tail; /* jump over first static link */
    for (; fal; fal = fal->tail) {
        formals = Tr_AccessList(Tr_Access(l, fal->head), formals);
    }

    return formals;
}

Tr_access Tr_allocLocal(Tr_level level, bool escape) {
    Tr_access acc = checked_malloc(sizeof(struct Tr_access_));
    acc->level = level;
    acc->access = F_allocLocal(level->frame, escape);
    return acc;
}

Tr_expList Tr_ExpList(Tr_exp head, Tr_expList tail) {
    // if (head == NULL) {
    //     EM_error(0, "head is NULL");
    // }
    // if (tail == NULL) {
    //     EM_error(0, "tail is NULL");
    // }
    Tr_expList el = checked_malloc(sizeof(struct Tr_expList_));
    el->head = head;
    el->tail = tail;
    return el;
}

/* Interface between translate and semantic */
Tr_exp Tr_simpleVar(Tr_access acc, Tr_level l) {
    T_exp addr = T_Temp(F_FP());
    for (; l != acc->level->parent; l = l->parent) {
        F_access sl = F_formals(l->frame)->head;
        addr = F_Exp(sl, addr);
    }
    return Tr_Ex(F_Exp(acc->access, addr));
}

Tr_exp Tr_fieldVar(Tr_exp base, int offset) {
    return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Const(offset * F_wordSize))));
}

Tr_exp Tr_subscriptVar(Tr_exp base, Tr_exp index) {
    return Tr_Ex(T_Mem(T_Binop(T_plus, unEx(base), T_Binop(T_mul, unEx(index), T_Const(F_wordSize)))));
}

Tr_exp Tr_nilExp(void) {
    //static Temp_temp tmp = NULL;
    //if (!tmp) {
    //    tmp = Temp_newtemp();
    //    T_stm alloc = T_Move(T_Temp(tmp),
    //            F_externalCall(String("initRecord"), T_ExpList(T_Const(0), NULL)));
    //    return Tr_Ex(T_Eseq(alloc, T_Temp(tmp)));
    //}
    //return Tr_Ex(T_Temp(tmp));
    return Tr_Ex(T_Const(0));
}

Tr_exp Tr_intExp(int i) {
    return Tr_Ex(T_Const(i));
}

static F_fragList stringFragList = NULL;
Tr_exp Tr_stringExp(string str) {
    Temp_label sl = Temp_newlabel();
    F_frag frag = F_StringFrag(sl, str);
    stringFragList = F_FragList(frag, stringFragList);
    return Tr_Ex(T_Name(sl));
}

Tr_exp Tr_callExp(Temp_label label, Tr_level funcLevel, Tr_level level, Tr_expList expList) {
    T_expList args = NULL;
    for (; expList; expList = expList->tail) {
        args = T_ExpList(unEx(expList->head), args);
    }
    args = T_ExpList(unEx(Tr_StaticLink(funcLevel, level)), args);
    return Tr_Ex(T_Call(T_Name(label), args));
}

Tr_exp Tr_binOpExp(A_oper oper, Tr_exp left, Tr_exp right) {
    T_binOp op;
    switch (oper) {
        case A_plusOp:
            op = T_plus;
            break;
        case A_minusOp:
            op = T_minus;
            break;
        case A_timesOp:
            op = T_mul;
            break;
        case A_divideOp:
            op = T_div;
            break;
        default:
            break;
    }
    return Tr_Ex(T_Binop(op, unEx(left), unEx(right)));
}

Tr_exp Tr_relOpExp(A_oper oper, Tr_exp left, Tr_exp right) {
    T_relOp op;
    switch (oper) {
        case A_ltOp:
            op = T_lt;
            break;
        case A_leOp:
            op = T_le;
            break;
        case A_gtOp:
            op = T_gt;
            break;
        case A_geOp:
            op = T_ge;
            break;
        case A_eqOp:
            op = T_eq;
            break;
        case A_neqOp:
            op = T_ne;
            break;
        default:
            break;
    }
    
    T_stm stm = T_Cjump(op, unEx(left), unEx(right), NULL, NULL);
    patchList trues = PatchList(&stm->u.CJUMP.true, NULL);
    patchList falses = PatchList(&stm->u.CJUMP.false, NULL);
    
    return Tr_Cx(trues, falses, stm);
}

Tr_exp Tr_recordExp(int num, Tr_expList fields) {
    Temp_temp r = Temp_newtemp();
    /* alloc (n * wordSize) memory */
    T_stm alloc = T_Move(T_Temp(r),
            F_externalCall(String("initRecord"), T_ExpList(T_Const(num * F_wordSize), NULL)));

    num--;
    T_stm seq = T_Move(T_Mem(T_Binop(T_plus, T_Temp(r), T_Const(num * F_wordSize))),
            unEx(fields->head));
    for (fields = fields->tail; fields && num >= 0; fields = fields->tail, num--) {
        seq = T_Seq(T_Move(T_Mem(T_Binop(T_plus, T_Temp(r), T_Const(num * F_wordSize))),
                    unEx(fields->head)), seq);
    }

    return Tr_Ex(T_Eseq(T_Seq(alloc, seq), T_Temp(r)));
}

Tr_exp Tr_seqExp(Tr_expList expList) {
    T_exp resl = unEx(expList->head);
    for (expList = expList->tail; expList; expList = expList->tail) {
        if (expList->head) {
            resl = T_Eseq(T_Exp(unEx(expList->head)), resl);
        }
    }
    return Tr_Ex(resl);
}

Tr_exp Tr_assignExp(Tr_exp left, Tr_exp right) {
    return Tr_Nx(T_Move(unEx(left), unEx(right)));
}

Tr_exp Tr_ifExp(Tr_exp test, Tr_exp then, Tr_exp elsee) {
    Temp_label trues = Temp_newlabel();
    Temp_label falses = Temp_newlabel();
    Temp_label join = Temp_newlabel();
    
    struct Cx cond = unCx(test);
    doPatch(cond.trues, trues);
    doPatch(cond.falses, falses);

    if (!elsee) {
        return Tr_Nx(T_Seq(cond.stm,
                        T_Seq(T_Label(trues),
                            T_Seq(unNx(then), T_Label(falses)))));
    } else {
        Temp_temp r = Temp_newtemp();
        T_stm joinJump = T_Jump(T_Name(join), Temp_LabelList(join, NULL));

        switch (then->kind) {
            case Tr_nx:
                return Tr_Nx(T_Seq(cond.stm,
                            T_Seq(T_Label(trues),
                                T_Seq(unNx(then),
                                    T_Seq(joinJump,
                                        T_Seq(T_Label(falses),
                                            T_Seq(unNx(elsee),
                                                T_Label(join))))))));
            case Tr_cx:
            case Tr_ex:
                return Tr_Ex(T_Eseq(T_Seq(cond.stm,
                                T_Seq(T_Label(trues),
                                    T_Seq(T_Move(T_Temp(r), unEx(then)),
                                        T_Seq(joinJump,
                                            T_Seq(T_Label(falses),
                                                T_Seq(T_Move(T_Temp(r), unEx(elsee)),
                                                    T_Label(join))))))),
                                                        T_Temp(r)));
            default:
                assert(0);
        }
    }
}

// Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body) {
//     Temp_label trues = Temp_newlabel();
//     Temp_label done = Temp_namedlabel(String("done"));
//     Temp_label testl = Temp_newlabel();
//     Cx ctest = unCx(test);
//     doPatch(ctest.trues, trues);
//     doPatch(ctest.falses, done);
//     T_stm stm;
//
//     stm = T_Seq(T_Label(testl),
//             T_Seq(ctest.stm,
//                 T_Seq(T_Seq(T_Label(trues),
//                     T_Seq(unNx(body), T_Jump(T_Name(testl), Temp_LabelList(testl, NULL)))), T_Label(done))));
//     return Tr_Nx(stm);
// }

Tr_exp Tr_whileExp(Tr_exp test, Tr_exp body, Tr_exp done) {
    Temp_label testl = Temp_newlabel(), bodyl = Temp_newlabel();
    return Tr_Nx(T_Seq(T_Label(testl),
                    T_Seq(T_Cjump(T_eq, unEx(test), T_Const(0), unEx(done)->u.NAME, bodyl),
                        T_Seq(T_Label(bodyl),
                            T_Seq(unNx(body),
                                T_Seq(T_Jump(T_Name(testl), Temp_LabelList(testl, NULL)),
                                    T_Label(unEx(done)->u.NAME)))))));
}

Tr_exp Tr_forExp(Tr_exp lo, Tr_exp hi, Tr_exp body, Tr_access acc) {
    T_exp tmp = T_Temp(F_FP());
    T_exp memt = F_Exp(acc->access, tmp);
    T_stm stm = T_Move(memt, unEx(lo));
    Temp_temp h = Temp_newtemp();
    T_stm limit = T_Move(T_Temp(h), unEx(hi));
    struct Cx cx;
    cx.stm = T_Cjump(T_lt, memt, T_Temp(h), NULL, NULL);
    cx.falses = PatchList(&cx.stm->u.CJUMP.false, NULL);
    cx.trues = PatchList(&cx.stm->u.CJUMP.true, NULL);
    T_stm finalBody = T_Seq(T_Move(memt, T_Binop(T_plus, memt, T_Const(1))), unNx(body));

    Tr_exp tmpWhile = Tr_whileExp(Tr_Cx(cx.trues, cx.falses, cx.stm), Tr_Nx(finalBody), Tr_doneExp());
    return Tr_Nx(T_Seq(stm, T_Seq(limit, unNx(tmpWhile))));
}

Tr_exp Tr_breakExp() {
    Temp_label done = Temp_namedlabel("done");
    return Tr_Nx(T_Jump(T_Name(done), Temp_LabelList(done, NULL)));
}

//Tr_exp Tr_breakExp(Tr_exp b) {
//    return Tr_Nx(T_Jump(T_Name(unEx(b)->u.NAME), Temp_LabelList(unEx(b)->u.NAME, NULL)));
//}

Tr_exp Tr_letExp(Tr_expList decs, Tr_exp body) {
    if (!decs) {
        return NULL;
    }
    T_stm stm = unNx(decs->head);
    for (decs = decs->tail; decs; decs = decs->tail) {
        stm = T_Seq(unNx(decs->head), stm);
    }
    if (body) {
        stm = T_Seq(stm, unNx(body));
    }
    return Tr_Nx(stm);
}

Tr_exp Tr_arrayExp(Tr_exp size, Tr_exp init) {
    return Tr_Ex(F_externalCall(String("initArray"),
                T_ExpList(unEx(size), T_ExpList(unEx(init), NULL))));
}

Tr_exp Tr_doneExp() {
    return Tr_Ex(T_Name(Temp_newlabel()));
}

Tr_exp Tr_StaticLink(Tr_level now, Tr_level def) {
    T_exp addr = T_Temp(F_FP());
    while (now && (now != def->parent)) {
        F_access sl = F_formals(now->frame)->head;
        addr = F_Exp(sl, addr);
        now = now->parent;
    }
    return Tr_Ex(addr);
}

Tr_exp Tr_functionDec(Tr_expList function) {
    T_stm stm = T_Move(T_Temp(F_RV()), unEx(function->head));
    for (function = function->tail; function; function = function->tail) {
        stm = T_Seq(T_Move(T_Temp(F_RV()), unEx(function->head)), stm);
    }
    return Tr_Nx(stm);
}

Tr_exp Tr_varDec(Tr_exp init, Tr_access acc) {
    return Tr_Nx(T_Move(F_Exp(acc->access, T_Temp(F_FP())), unEx(init)));
}

Tr_exp Tr_typeDec(Tr_expList type) {
    return Tr_Ex(T_Const(0));
}

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

static F_fragList fragList = NULL;
void Tr_procEntryExit(Tr_level level, Tr_exp body, Tr_accessList formals) {
    // EM_error(0, "Tr_procEntryExit");
    F_frag procFrag = F_ProcFrag(unNx(body), level->frame);
    fragList = F_FragList(procFrag, fragList);
}

F_fragList Tr_getResult(void) {
    F_fragList cur = NULL, prev = NULL;
    for (cur = stringFragList; cur; cur = cur->tail) {
        prev = cur;
    }
    if (prev) {
        prev->tail = fragList;
    }
    return stringFragList ? stringFragList : fragList;
}
