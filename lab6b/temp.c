/*
 * temp.c - functions to create and manipulate temporary variables which are
 *          used in the IR tree representation before it has been determined
 *          which variables are to go into registers.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"

struct Temp_temp_ {int num;};

string Temp_labelstring(Temp_label s)
{return S_name(s);
}

static int labels = 0;

Temp_label Temp_newlabel(void)
{char buf[100];
 sprintf(buf,"L%d",labels++);
 return Temp_namedlabel(String(buf));
}

/* The label will be created only if it is not found. */
Temp_label Temp_namedlabel(string s)
{return S_Symbol(s);
}

static int temps = 100;

Temp_temp Temp_newtemp(void)
{Temp_temp p = (Temp_temp) checked_malloc(sizeof (*p));
 p->num=temps++;
 {char r[16];
  sprintf(r, "%d", p->num);
  Temp_enter(Temp_name(), p, String(r));
 }
 return p;
}



struct Temp_map_ {TAB_table tab; Temp_map under;};


Temp_map Temp_name(void) {
 static Temp_map m = NULL;
 if (!m) m=Temp_empty();
 return m;
}

Temp_map newMap(TAB_table tab, Temp_map under) {
  Temp_map m = checked_malloc(sizeof(*m));
  m->tab=tab;
  m->under=under;
  return m;
}

Temp_map Temp_empty(void) {
  return newMap(TAB_empty(), NULL);
}

Temp_map Temp_layerMap(Temp_map over, Temp_map under) {
  if (over==NULL)
      return under;
  else return newMap(over->tab, Temp_layerMap(over->under, under));
}
//-x626e70
void Temp_enter(Temp_map m, Temp_temp t, string s) {
  assert(m && m->tab);
  TAB_enter(m->tab,t,s);
}

string Temp_look(Temp_map m, Temp_temp t) {
  //Temp_dumpMap(stdout, m);
  //printf("======================================================================\n");
  string s;
  assert(m && m->tab);
  s = TAB_look(m->tab, t);
  //printf("Temp_look s = %s\n", s);
  if (s) return s;
  else if (m->under) return Temp_look(m->under, t);
  else return NULL;
}

Temp_tempList Temp_TempList(Temp_temp h, Temp_tempList t) 
{Temp_tempList p = (Temp_tempList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

int Temp_listSize(Temp_tempList tl) {
    int i = 0;
    for (; tl; tl = tl->tail) {
        i++;
    }
    return i;
}

Temp_labelList Temp_LabelList(Temp_label h, Temp_labelList t)
{Temp_labelList p = (Temp_labelList) checked_malloc(sizeof (*p));
 p->head=h; p->tail=t;
 return p;
}

static FILE *outfile;
void showit(Temp_temp t, string r) {
  fprintf(outfile, "t%d -> %s\n", t->num, r);
}

void Temp_dumpMap(FILE *out, Temp_map m) {
  outfile=out;
  TAB_dump(m->tab,(void (*)(void *, void*))showit);
  if (m->under) {
     fprintf(out,"---------\n");
     Temp_dumpMap(out,m->under);
  }
}

bool Temp_isEqual(Temp_tempList left, Temp_tempList right) {
    if (!left && !right) {
        return TRUE;
    }

    Temp_tempList iter, stay = right;
    bool result = FALSE;

    for (; left && right; left = left->tail, right = right->tail) {
        for (iter = stay; iter; iter = iter->tail) {
            if (left->head == iter->head) {
                result = TRUE;
                break;
            }
        }
        if (!result) {
            return FALSE;
        }
    }

    if (left || right) {
        result = FALSE;
    }
    return result;
}

Temp_tempList Temp_union(Temp_tempList left, Temp_tempList right) {
    Temp_tempList unionn = Temp_deepCopy(left), iter,  tmp;
    for (iter = right; iter; iter = iter->tail) {
        for (tmp = left; tmp; tmp = tmp->tail) {
            if (tmp->head->num == iter->head->num) {
                break;
            }
        }
        if (!tmp) {
            unionn = Temp_TempList(iter->head, unionn);
        }
    }
    return unionn;
}

Temp_tempList Temp_subtraction(Temp_tempList left, Temp_tempList right) {
    Temp_tempList subtraction = NULL, iter, tmp;
    for (iter = left; iter; iter = iter->tail) {
        for (tmp = right; tmp; tmp = tmp->tail) {
            if (tmp->head->num == iter->head->num) {
                break;
            }
        }
        if (!tmp) {
            subtraction = Temp_TempList(iter->head, subtraction);
        }
    }
    return subtraction;
}

Temp_tempList Temp_deepCopy(Temp_tempList origin) {
    Temp_tempList copy = NULL, iter;
    for (iter = origin; iter; iter = iter->tail) {
        copy = Temp_TempList(iter->head, copy);
    }
    return copy;
}

void Temp_printList(Temp_tempList tl) {
    printf("Elements: ");
    Temp_tempList iter = tl;
    for (; tl; tl = tl->tail) {
        printf("%d ", tl->head->num);
    }
    printf("\n");
}
