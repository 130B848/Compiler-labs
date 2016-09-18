/* This file is not complete.  You should fill it in with your
   solution to the programming exercise. */
#include <stdio.h>
#include "prog1.h"
#include "slp.h"
int maxargs(A_stm stm);
void interp(A_stm stm);

int maxargs(A_stm stm) {
	switch (stm->kind) {
		case A_compoundStm:

			break;
		case A_assignStm:

			break;
		case A_printStm:

			break;
		default:
			break;
	}
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
