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
#include "errormsg.h"
#include "table.h"


Temp_tempList FG_def(G_node n) {
	//your code here.
    AS_instr info = G_nodeInfo(n);
    switch (info->kind) {
        case I_OPER:
            return info->u.OPER.dst;
        case I_LABEL:
            return NULL;
        case I_MOVE:
            return info->u.MOVE.dst;
        default:
            assert(0 && "FG_def");
    }
}

Temp_tempList FG_use(G_node n) {
	//your code here.
    AS_instr info = G_nodeInfo(n);
    switch (info->kind) {
        case I_OPER:
            return info->u.OPER.src;
        case I_LABEL:
            return NULL;
        case I_MOVE:
            return info->u.MOVE.src;
        default:
            assert(0 && "FG_use");
    }
}

bool FG_isMove(G_node n) {
	//your code here.
    AS_instr info = G_nodeInfo(n);
    return (info && info->kind == I_MOVE);
}

G_graph FG_AssemFlowGraph(AS_instrList il) {
    TAB_table labels = TAB_empty();
    //printf("FG_AssemFlowGraph labels: 0x%x\n", labels);
    G_graph graph = G_Graph();

    //add node and label
    AS_instrList iter;
    for (iter = il; iter; iter = iter->tail) {
        G_node node = G_Node(graph, iter->head);
        if (iter->head->kind == T_LABEL) {
            TAB_enter(labels, iter->head->u.LABEL.label, node);
        }
    }

    //add edge
    G_nodeList nl;
    for (nl = G_nodes(graph); nl; nl = nl->tail) {
        AS_instr info = G_nodeInfo(nl->head);
        AS_targets next = info->u.OPER.jumps;
        if (!next) {
            if (nl->tail) {
                G_addEdge(nl->head, nl->tail->head);
            }
        } else {
            Temp_labelList ll;
            for (ll = next->labels; ll; ll = ll->tail) {
                G_addEdge(nl->head, TAB_look(labels, ll->head));
            }
        }
    }

    return graph;
}

//static inline void FG_addJumpEdge(G_table t, G_node n) {
//    AS_instr info = G_nodeInfo(n);
//    if (!info->u.OPER.jumps) {
//        return;
//    }
//    Temp_labelList ll = info->u.OPER.jumps->labels;
//    G_node target = NULL;
//    for (; ll; ll = ll->tail) {
//        target = TAB_look(t, ll->head);
//        if (target && !G_goesTo(n, target)) {
//            G_addEdge(n, target);
//        }
//    }
//}
//
//G_graph FG_AssemFlowGraph(AS_instrList il) {
//	//your code here.
//    G_graph graph = G_Graph();
//    G_node curr = NULL, prev = NULL;
//    G_nodeList nodeList = NULL;
//    TAB_table labels = TAB_empty();
//    AS_instr i;
//
//    for (; il; il = il->tail) {
//        i = il->head;
//        curr = G_Node(graph, i);
//        if (prev) {
//            G_addEdge(prev, curr);
//        }
//        prev = curr;
//        switch (i->kind) {
//            case I_OPER:
//                nodeList = G_NodeList(curr, nodeList);
//                break;
//            case I_LABEL:
//                TAB_enter(labels, i->u.LABEL.label, curr);
//                break;
//            case I_MOVE:
//                break;
//            default:
//                assert(0);
//        }
//    }
//
//    for (; nodeList; nodeList = nodeList->tail) {
//        FG_addJumpEdge(labels, nodeList->head);
//    }
//
//    return graph;
//}
