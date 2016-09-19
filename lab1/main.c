/* This file is not complete.  You should fill it in with your
   solution to the programming exercise. */
#include <stdio.h>
#include <string.h>
#include "prog1.h"
#include "slp.h"
int maxargs(A_stm stm);
void interp(A_stm stm);

typedef struct table *Table_;
struct table {string id; int value; Table_ tail;};
struct IntAndTable {int value; Table_ t;};

Table_ Table(string id, int value, struct table *tail);
Table_ interpStm(A_stm stm, Table_ t);
int lookup(Table_ t, string key);
struct IntAndTable interpExp(A_exp exp, Table_ t);
int expListArgs(A_expList expList);
Table_ interpPrintStm(A_stm stm, Table_ t);
struct IntAndTable interpOpExp(A_exp opExp, Table_ t);
void interp(A_stm stm);

Table_ Table(string id, int value, struct table *tail) {
 Table_ t = checked_malloc(sizeof(struct table));
 t->id = id;
 t->value = value;
 t->tail = tail;
 return t;
}

Table_ interpStm(A_stm stm, Table_ t) {
  Table_ tmp;
	struct IntAndTable iat;
	A_expList expList;

	switch (stm->kind) {
		case A_compoundStm:
			tmp = interpStm(stm->u.compound.stm1, t);
			return interpStm(stm->u.compound.stm2, tmp);
		case A_assignStm:
			iat = interpExp(stm->u.assign.exp, t);
			return Table(stm->u.assign.id, iat.value, iat.t);
		case A_printStm:
			expList = stm->u.print.exps;
			while (expList->kind == A_pairExpList) {
				iat = interpExp(expList->u.pair.head, t);
				printf("%d ", iat.value);
				t = iat.t;
				expList = expList->u.pair.tail;
			}
			iat = interpExp(expList->u.last, t);
			printf("%d\n", iat.value);
			return iat.t;
		default:
			break;
	}
}

int lookup(Table_ t, string key) {
	while (t) {
		if (!strcmp(t->id, key)) {
			return t->value;
		}
		t = t->tail;
	}
	printf("%s\n", "Look up error.");
	return -1;
}

struct IntAndTable interpExp(A_exp exp, Table_ t) {
  Table_ tmp;
	struct IntAndTable iat;

	switch (exp->kind) {
		case A_idExp:
			iat.value = lookup(t, exp->u.id);
			iat.t = t;
			return iat;
		case A_numExp:
			iat.value = exp->u.num;
			iat.t = t;
			return iat;
		case A_opExp:
			return interpOpExp(exp, t);
		case A_eseqExp:
			tmp = interpStm(exp->u.eseq.stm, t);
			return interpExp(exp->u.eseq.exp, tmp);
		default:
			break;
	}
}

struct IntAndTable interpOpExp(A_exp opExp, Table_ t) {
	struct IntAndTable tmp, iat;
	tmp = interpExp(opExp->u.op.left, t);
	iat = interpExp(opExp->u.op.right, tmp.t);

	switch (opExp->u.op.oper) {
		case A_plus:
			iat.value += tmp.value;
			break;
		case A_minus:
			iat.value = tmp.value - iat.value;
			break;
		case A_times:
			iat.value *= tmp.value;
			break;
		case A_div:
			iat.value = tmp.value / iat.value;
			break;
		default:
			printf("%s\n", "InterpOpExp error.");
			break;
	}
	return iat;
}

void interp(A_stm stm) {
  interpStm(stm, NULL);
}

int flag, tmp;

int expArgs(A_exp exp) {
	if (exp->kind == A_eseqExp) {
		return maxargs(exp->u.eseq.stm) + expArgs(exp->u.eseq.exp);
	} else {
		return flag ? 1 : 0;
	}
}

int expListArgs(A_expList expList) {
	switch (expList->kind) {
		case A_pairExpList:
			return expArgs(expList->u.pair.head) + expListArgs(expList->u.pair.tail);
		case A_lastExpList:
      if (flag) {
        tmp = expArgs(expList->u.last);
        flag--;
        return tmp;
      }
      return 0;
		default:
			break;
	}
	return 0;
}

int maxargs(A_stm stm) {
  flag = 0;
	switch (stm->kind) {
		case A_compoundStm:
			return maxargs(stm->u.compound.stm1) + maxargs(stm->u.compound.stm2);
		case A_assignStm:
			return expArgs(stm->u.assign.exp);
		case A_printStm:
      flag++;
			return expListArgs(stm->u.print.exps);
		default:
			break;
	}
	return 0;
}

/*
 *Please don't modify the main() function
 */
int main()
{
	int args;

	printf("prog\n");
	args = maxargs(prog());
	printf("args: %d\n",args);
	interp(prog());

	printf("prog_prog\n");
	args = maxargs(prog_prog());
	printf("args: %d\n",args);
	interp(prog_prog());

	printf("right_prog\n");
	args = maxargs(right_prog());
	printf("args: %d\n",args);
	interp(right_prog());

	return 0;
}
