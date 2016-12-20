#include <stdio.h>
#include <stdlib.h>
#include "util.h"
#include "symbol.h"
#include "absyn.h"
#include "temp.h"
#include "errormsg.h"
#include "tree.h"
#include "assem.h"
#include "frame.h"
#include "codegen.h"
#include "table.h"

//Lab 6: your code here
static Temp_temp munchExp(T_exp e);
static Temp_tempList munchArgs(int i, T_expList args);
static void munchStm(T_stm s);
static void emit(AS_instr inst);

static inline string op2asm(T_binOp op) {
    switch (op) {
        case T_plus: {
            return "add";
        }
        case T_minus: {
            return "sub";
        }
        case T_mul: {
            return "imul";
        }
        case T_div: {
            return "div";
        }
        case T_and: {
            return "and";
        }
        case T_or: {
            return "or";
        }
        case T_lshift: {
            return "shl";
        }
        case T_rshift: {
            return "shr";
        }
        case T_arshift: {
            return "sar";
        }
        case T_xor: {
            return "xor";
        }
    }
}

static Temp_temp munchExp(T_exp e) {
    Temp_temp r = Temp_newtemp();
    char asm_str[100];

    switch (e->kind) {
        case T_BINOP: {
            T_exp left = e->u.BINOP.left, right = e->u.BINOP.right;
            if (left->kind == T_CONST) { /* BINOP(op, e1, CONST(i)) */
                sprintf(asm_str, "%s $0x%x, `d0\n", op2asm(e->u.BINOP.op), left->u.CONST);
                emit(AS_Oper(String(asm_str), Temp_TempList(r, NULL),
                                          Temp_TempList(munchExp(right), NULL),
                                          NULL));
            } else if (right->kind == T_CONST) { /* BINOP(op, e1, CONST(i)) */
                sprintf(asm_str, "%s $0x%x, `d0\n", op2asm(e->u.BINOP.op), right->u.CONST);
                emit(AS_Oper(String(asm_str), Temp_TempList(r, NULL),
                                          Temp_TempList(munchExp(left), NULL),
                                          NULL));
            } else { /* BINOP(op, e1, e2) */
                sprintf(asm_str, "%s `s0, `d0\n", op2asm(e->u.BINOP.op));
                emit(AS_Oper(String(asm_str), Temp_TempList(r, NULL),
                                          Temp_TempList(munchExp(left),
                                          Temp_TempList(munchExp(right), NULL)),
                                          NULL));
            }
            return r;
        }
        case T_MEM: {
            T_exp mem = e->u.MEM;
            if (mem->kind == T_BINOP && mem->u.BINOP.op == T_plus) { /* MEM(BINOP(+, CONST, e) */
                T_exp left = mem->u.BINOP.left, right = mem->u.BINOP.right;
                T_exp ct = (left->kind == T_CONST) ? left : right;
                sprintf(asm_str, "mov 0x%x(`s0), `d0\n", ct->u.CONST);
                emit(AS_Oper(String(asm_str), Temp_TempList(r, NULL),
                                          Temp_TempList(munchExp(ct), NULL),
                                          NULL));
            } else if (mem->kind == T_CONST) {
                sprintf(asm_str, "mov (0x%x), `d0\n", mem->u.CONST);
                emit(AS_Oper(String(asm_str), Temp_TempList(r, NULL), NULL, NULL));
            } else {
                emit(AS_Oper(String("mov (`s0), `d0\n"), Temp_TempList(r, NULL),
                                                       Temp_TempList(munchExp(mem->u.MEM), NULL),
                                                       NULL));
            }
            return r;
        }
        case T_TEMP: {
            return e->u.TEMP;
        }
        case T_ESEQ: {
            munchStm(e->u.ESEQ.stm);
            return munchExp(e->u.ESEQ.exp);
        }
        case T_NAME: {
            sprintf(asm_str, "mov %s, `d0\n", Temp_labelstring(e->u.NAME));
            return r;
        }
        case T_CONST: {
            sprintf(asm_str, "mov $0x%x, `d0\n", e->u.CONST);
            emit(AS_Oper(String(asm_str), Temp_TempList(r, NULL), NULL, NULL));
            return r;
        }
        case T_CALL: {
            T_exp fun = e->u.CALL.fun;
            r = munchExp(fun);
            Temp_tempList l = munchArgs(0, e->u.CALL.args);
            emit(AS_Oper(String("call `s0\n"), F_calldefs(), Temp_TempList(r, l), NULL));
            return r;
        }
    }
}

static Temp_tempList munchArgs(int i, T_expList args) {
    if (!args) {
        return NULL;
    }

    Temp_tempList tlist = munchArgs(i + 1, args->tail);
    Temp_temp rarg = munchExp(args->head);
    emit(AS_Oper(String("push `s0\n"), NULL, Temp_TempList(rarg, NULL), NULL));
    return Temp_TempList(rarg, tlist);
}

static inline string cmp2asm(T_relOp op) {
    switch (op) {
        case T_eq:
            return "je";
        case T_ne:
            return "jne";
        case T_lt:
            return "jl";
        case T_gt:
            return "jg";
        case T_le:
            return "jle";
        case T_ge:
            return "jge";
        case T_ult:
            return "jb";
        case T_ule:
            return "jbe";
        case T_ugt:
            return "ja";
        case T_uge:
            return "jae";
    }
}

static void munchStm(T_stm s) {
    char asm_str[100];

    switch (s->kind) {
        case T_SEQ: {
            munchStm(s->u.SEQ.left);
            munchStm(s->u.SEQ.right);
            break;
        }
        case T_LABEL: {
            sprintf(asm_str, "%s\n", Temp_labelstring(s->u.LABEL));
            emit(AS_Label(String(asm_str), s->u.LABEL));
            break;
        }
        case T_JUMP: {
            Temp_temp r = munchExp(s->u.JUMP.exp);
            emit(AS_Oper(String("jmp `d0\n"), Temp_TempList(r, NULL),
                        NULL, AS_Targets(s->u.JUMP.jumps)));
            break;
        }
        case T_CJUMP: {
            Temp_temp left = munchExp(s->u.CJUMP.left), right = munchExp(s->u.CJUMP.right);
            emit(AS_Oper(String("cmp `s0, `s1\n"), NULL,
                        Temp_TempList(left, Temp_TempList(right, NULL)), NULL));
            sprintf(asm_str, "%s `j0\n", cmp2asm(s->u.CJUMP.op));
            emit(AS_Oper(String(asm_str), NULL, NULL,
                        AS_Targets(Temp_LabelList(s->u.CJUMP.true, NULL))));
            break;
        }
        case T_MOVE: {
            T_exp dst = s->u.MOVE.dst, src = s->u.MOVE.src;
            if (dst->kind == T_MEM) {
                if (dst->u.MEM->kind == T_BINOP && dst->u.MEM->u.BINOP.op == T_plus) {
                    /* MOVE(MEM(BINOP(+, e1, CONST)), e2) */
                    if (dst->u.MEM->u.BINOP.right->kind == T_CONST) {
                        sprintf(asm_str, "mov `s1, 0x%x(`s0)\n", dst->u.MEM->u.BINOP.right->u.CONST);
                        emit(AS_Oper(String(asm_str), NULL,
                                    Temp_TempList(munchExp(dst->u.MEM->u.BINOP.left),
                                    Temp_TempList(munchExp(src), NULL)), NULL));
                    }
                    /* MOVE(MEM(BINOP(+, CONST, e1)), e2) */
                    if (dst->u.MEM->u.BINOP.left->kind == T_CONST) {
                        sprintf(asm_str, "mov `s1, 0x%x(`s0)\n", dst->u.MEM->u.BINOP.left->u.CONST);
                        emit(AS_Oper(String(asm_str), NULL,
                                    Temp_TempList(munchExp(dst->u.MEM->u.BINOP.right),
                                    Temp_TempList(munchExp(src), NULL)), NULL));
                    }
                } else if (src->kind == T_MEM) { /* MOVE(MEM(e1), MEM(e2)) */
                    emit(AS_Oper(String("mov `s1, (`s0)\n"), NULL,
                                Temp_TempList(munchExp(dst->u.MEM),
                                Temp_TempList(munchExp(src->u.MEM), NULL)), NULL));

                } else if (dst->u.MEM->kind == T_CONST) { /* MOVE(MEM(CONST), e) */
                    sprintf(asm_str, "mov `s0, (0x%x)\n", dst->u.MEM->u.CONST);
                    emit(AS_Oper(String(asm_str), NULL, Temp_TempList(munchExp(src), NULL), NULL));
                } else { /* MOVE(MEM(e1), e2) */
                    emit(AS_Oper(String("mov `s1, (`s0)\n"), NULL,
                            Temp_TempList(munchExp(dst->u.MEM),
                            Temp_TempList(munchExp(src), NULL)), NULL));
                }
            } else { /* MOVE(TEMP(e1), e2) */
                emit(AS_Move(String("mov `s0, `d0\n"),
                            Temp_TempList(munchExp(dst), NULL),
                            Temp_TempList(munchExp(src), NULL)));
            }
            break;
        }
        case T_EXP: {
            munchExp(s->u.EXP);
            break;
        }
    }
}

static AS_instrList iList = NULL, last = NULL;
static void emit(AS_instr inst) {
    if (last) {
        last = last->tail = AS_InstrList(inst, NULL);
    } else {
        last = iList  = AS_InstrList(inst, NULL);
    }
}

AS_instrList F_codegen(F_frame f, T_stmList stmList) {
    AS_instrList list;
    T_stmList sl;

    /* TODO: miscellaneous initializations as necessary */

    for (sl = stmList; sl; sl = sl->tail) {
        munchStm(sl->head);
    }
    list = iList;
    iList = last = NULL;
    return list;
}
