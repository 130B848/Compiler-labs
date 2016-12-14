#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "tree.h"
#include "frame.h"

/*Lab5: Your implementation here.*/
struct F_frame_ {
    F_accessList formals;
    F_accessList locals;
    /* TODO: instructions required to implement the "view shift" */
    int frameSize;
    Temp_label name;
};

struct F_access_ {
    enum {
        inFrame, inReg
    } kind;
    union {
        int offset; /* InFrame */
        Temp_temp reg; /* InReg */
    } u;
};

static F_access InFrame(int offset) {
    F_access acc = checked_malloc(sizeof(struct F_access_));
    acc->kind = inFrame;
    acc->u.offset = offset;
    return acc;
}

static F_access InReg(Temp_temp reg) {
    F_access acc = checked_malloc(sizeof(struct F_access_));
    acc->kind = inReg;
    acc->u.reg = reg;
    return acc;
}

F_accessList F_AccessList(F_access head, F_accessList tail) {
    F_accessList al = (F_accessList)checked_malloc(sizeof(struct F_accessList_));
    al->head = head;
    al->tail = tail;
    return al;
}

F_frag F_StringFrag(Temp_label label, string str) {
	F_frag strFrag = checked_malloc(sizeof(struct F_frag_));
    strFrag->kind = F_stringFrag;
    strFrag->u.stringg.label = label;
    strFrag->u.stringg.str = str;
    return strFrag;
}

F_frag F_ProcFrag(T_stm body, F_frame frame) {
	F_frag procFrag = checked_malloc(sizeof(struct F_frag_));
    procFrag->kind = F_procFrag;
    procFrag->u.proc.body = body;
    procFrag->u.proc.frame = frame;
    return procFrag;
}

F_fragList F_FragList(F_frag head, F_fragList tail) {
	F_fragList fl = checked_malloc(sizeof(struct F_fragList_));
    fl->head = head;
    fl->tail = tail;
    return fl;
}

static Temp_tempList callersaves() {
    Temp_temp ecx = Temp_newtemp(), edx = Temp_newtemp();
    if (!F_tempMap) {
        F_tempMap = Temp_empty();
    }
    //Temp_enter(F_tempMap, ecx, "ecx");
    //Temp_enter(F_tempMap, edx, "edx");
    return Temp_TempList(F_RV(), Temp_TempList(ecx, Temp_TempList(edx, NULL)));
}

static Temp_tempList calleesaves() {
    Temp_temp ebx = Temp_newtemp(), edi = Temp_newtemp(), esi = Temp_newtemp();
    if (!F_tempMap) {
        F_tempMap = Temp_empty();
    }
    //Temp_enter(F_tempMap, ebx, "ebx");
    //Temp_enter(F_tempMap, edi, "edi");
    //Temp_enter(F_tempMap, esi, "esi");
    return Temp_TempList(ebx, Temp_TempList(edi, Temp_TempList(esi, NULL)));
}

static Temp_tempList specRegs = NULL;
static Temp_tempList specialregs() {
    if (!specRegs) {
        specRegs = Temp_TempList(F_FP(), Temp_TempList(F_SP(), Temp_TempList(F_RV(), NULL)));
    }
    return specRegs;
}

Temp_tempList F_registers(void) {
    return Temp_union(specialregs(), Temp_union(callersaves(), calleesaves()));
}

string F_getlabel(F_frame frame) {
    return S_name(frame->name);
}

T_exp F_Exp(F_access acc, T_exp framePtr) {
    if (acc->kind == inFrame) {
        return T_Mem(T_Binop(T_plus, framePtr, T_Const(acc->u.offset)));
    } else {
        return T_Temp(acc->u.reg);
    }
}

F_access F_allocLocal(F_frame f, bool escape) {
    F_access acc;
    if (escape) {
        acc = InFrame(f->frameSize);
        f->frameSize += F_wordSize;
    } else {
        acc = InReg(Temp_newtemp());
    }
    f->locals = F_AccessList(acc, f->locals);
    return acc;
}

F_accessList F_formals(F_frame f) {
    return f->formals;
}

Temp_label F_name(F_frame f) {
    return f->name;
}

const int F_wordSize = 4;

static Temp_temp framePtr = NULL;
Temp_temp F_FP(void) {
    if (!framePtr) {
        if (!F_tempMap) {
            F_tempMap = Temp_empty();
        }
        framePtr = Temp_newtemp();
        Temp_enter(F_tempMap, framePtr, "ebp");
    }
    return framePtr;
}

static Temp_temp stackPtr = NULL;
Temp_temp F_SP(void) {
    if (!stackPtr) {
        if (!F_tempMap) {
            F_tempMap = Temp_empty();
        }
        stackPtr = Temp_newtemp();
        Temp_enter(F_tempMap, stackPtr, "esp");
    }
    return stackPtr;
}

static Temp_temp returnVal = NULL;
Temp_temp F_RV(void) {
    if (!returnVal) {
        if (!F_tempMap) {
            F_tempMap = Temp_empty();
        }
        returnVal = Temp_newtemp();
        Temp_enter(F_tempMap, returnVal, "eax");
    }
    return returnVal;
}

F_frame F_newFrame(Temp_label name, U_boolList formals) {
    F_frame f = (F_frame)checked_malloc(sizeof(struct F_frame_));
    f->name = name;
    f->formals = NULL;
    F_access tmp = NULL;
    for (; formals; formals = formals->tail) {
        if (!formals->head) {
            tmp = InReg(Temp_newtemp());
        } else {
            tmp = InFrame(f->frameSize);
            f->frameSize += F_wordSize;
        }

        f->formals = F_AccessList(tmp, f->formals);
    }
    return f;
}

Temp_tempList F_calldefs() {
    static Temp_tempList regs = NULL;
    return regs ? regs : (regs = callersaves());
}

T_exp F_externalCall(string s, T_expList args) {
    return T_Call(T_Name(Temp_namedlabel(s)), args);
}

T_stm F_procEntryExit1(F_frame frame, T_stm stm) {
    return stm;
}

static Temp_tempList returnSink = NULL;
AS_instrList F_procEntryExit2(AS_instrList body) {
    if (!returnSink) {
        returnSink = Temp_TempList(F_SP(), calleesaves());
    }
    return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));
}

AS_proc F_procEntryExit3(F_frame frame, AS_instrList body) {
    char buf[100];
    sprintf(buf, "PROCEDURE %s\n", S_name(frame->name));
    return AS_Proc(String(buf), body, "END\n");
}
