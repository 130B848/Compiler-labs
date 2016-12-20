#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"

G_table liveMap;
TAB_table tNode;

Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail) {
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));
	lm->src = src;
	lm->dst = dst;
	lm->tail = tail;
	return lm;
}

Temp_temp Live_gtemp(G_node n) {
	//your code here.
    return (Temp_temp)G_nodeInfo(n);
}

static void calculateLiveness(G_graph flow) {
    bool done = FALSE;
    G_nodeList nodeList = G_nodes(flow), iter;
    G_table in, out, def, use;

    //initialize liveness info
    in = G_empty();
    out = G_empty();
    //printf("liveness in: 0x%x out: 0x%x\n", in, out);
    for (iter = nodeList; iter; iter = iter->tail) {
        G_enter(in, iter->head, NULL);
        G_enter(out, iter->head, NULL);
    }

    do {
        done = TRUE;
        for (iter = nodeList; iter; iter = iter->tail) {
            //traverse all the instructions in the flow graph
            Temp_tempList oldIn = G_look(in, iter->head), oldOut = G_look(out, iter->head);
            
            //in[n] = use[n] U (out[n] - def[n])
            Temp_tempList newIn = Temp_union(FG_use(iter->head), 
                                    Temp_subtraction(oldOut, FG_def(iter->head)));
            //printf("newIn ");
            //Temp_printList(newIn);
            //printf("oldIn ");
            //Temp_printList(oldIn);
            //printf("\n");
            if (!Temp_isEqual(newIn, oldIn)) {
                done = FALSE;
            }
            G_enter(in, iter->head, newIn);

            //out[n] = U in[s] { s, s->succ[n] }
            Temp_tempList newOut = NULL;
            G_nodeList succ = G_succ(iter->head);
            for (; succ; succ = succ->tail) {
                newOut = Temp_union(newOut, G_look(in, succ->head));
            }
            //printf("newOut ");
            //Temp_printList(newOut);
            //printf("oldOut ");
            //Temp_printList(oldOut);
            //printf("\n");
            if (!Temp_isEqual(newOut, oldOut)) {
                done = FALSE;
            }
            G_enter(out, iter->head, newOut);
        }
        //printf("=================================================flag done is %d\n\n", done);
    } while (!done);

    //build liveMap
    liveMap = G_empty();
    //printf("liveness liveMap: 0x%x\n", liveMap);
    for (iter = nodeList; iter; iter = iter->tail) {
        Temp_tempList list = NULL;
        Temp_tempList oldOut = G_look(out, iter->head), tmp;
        for (tmp = oldOut; tmp; tmp = tmp->tail) {
            list = Temp_TempList(tmp->head, list);
        }
        if (list) {
            G_enter(liveMap, iter->head, list);
        }
    }
}

static G_graph buildGraph(G_graph flow) {
    Temp_tempList temps = NULL;
    G_nodeList nodeList = G_nodes(flow), iter;
    
    for (iter = nodeList; iter; iter = iter->tail) {
        Temp_tempList uses = FG_use(iter->head), defs = FG_def(iter->head);
        for (; uses; uses = uses->tail) {
            temps = Temp_TempList(uses->head, temps);
        }
        for (; defs; defs = defs->tail) {
            temps = Temp_TempList(defs->head, temps);
        }
    }

    G_graph g = G_Graph();
    tNode = TAB_empty();
    //printf("buildGraph tNode: 0x%x\n", tNode);
    for (; temps; temps = temps->tail) {
        TAB_enter(tNode, temps->head, G_Node(g, temps->head));
    }
    
    //builde interference graph
    Temp_tempList i, j;
    //traverse every instruction
    for (iter = nodeList; iter; iter = iter->tail) {
        //traverse variables defined in the instructions
        for (i = FG_def(iter->head); i; i = i->tail) {
            //traverse every live variables
            for (j = G_look(liveMap, iter->head); j; j = j->tail) {
                if (i->head != j->head && //prevent self-cycle
                        !(FG_isMove(iter->head) && FG_use(iter->head)->head == j->head)) {
                    G_addEdge(TAB_look(tNode, i->head), TAB_look(tNode, j->head));
                    G_addEdge(TAB_look(tNode, j->head), TAB_look(tNode, i->head));
                }
            }
        }
    }

    return g;
}

struct Live_graph Live_liveness(G_graph flow) {
	//your code here.
	struct Live_graph lg;
    calculateLiveness(flow);
    lg.graph = buildGraph(flow);
	return lg;
}

