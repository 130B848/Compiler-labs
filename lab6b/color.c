#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "color.h"
#include "table.h"
#include "liveness.h"

struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs) {
	//your code here.
	struct COL_result ret;
    ret.coloring = Temp_empty();
 
    //Temp_dumpMap(stdout, initial);
    //printf("======================================================================\n");

    int number = 0;
    G_nodeList stackTop = NULL;

    //traverse every temporary variable node
    G_nodeList iter;
    for (iter = G_nodes(ig); iter; iter = iter->tail) {
        number++;
        //get the corresponding temporary of the node
        Temp_temp temp = Live_gtemp(iter->head);
        //if temp has been assigned registers
        if (!Temp_look(initial, temp)) {
            number--;
            stackTop = G_NodeList(iter->head, stackTop); //push node into stack
            Temp_enter(ret.coloring, temp, (string)temp); //push temp into map, their reg is themselves
            //Temp_dumpMap(stdout, ret.coloring);
            //printf("====================================================================\n");
            //delete all the edge out from this node
            G_nodeList adj;
            for (adj = G_succ(iter->head); adj; adj = adj->tail) {
                G_rmEdge(iter->head, adj->head);
            }
        }
    }

    //now remains 'number' temps that have no reg-assign
    int i;
    for (i = 0; i < number; i++) {
        G_node node = NULL;
        int max = -1;
        //traverse each temporay node again
        G_nodeList iter;
        for (iter = G_nodes(ig); iter; iter = iter->tail) {
            //printf("init.tempMap = %d\n", Temp_look(initial, Live_gtemp(iter->head)));
            //printf("G_inNodeList = %d\n", G_inNodeList(iter->head, stackTop));
            //printf("=============================================================\n");
            if (!Temp_look(initial, Live_gtemp(iter->head)) &&
                    !G_inNodeList(iter->head, stackTop)) {
                int num = outDegree(iter->head);
                //printf("out degree = %d\n", num);
                if (max < num && num < Temp_listSize(regs)) { 
                    //find a max out-degree and small than reg num
                    max = num;
                    node = iter->head;
                }
            }
        }

        if (!node) {
            //TODO: spill
            printf("COL_color: register spill!!!\n");
            break;
        }
        //if no spill, push into stack and remove all edges from non-stack-node to this node
        stackTop = G_NodeList(node, stackTop);
        G_nodeList adj;
        for (adj = G_pred(node); adj; adj = adj->tail) {
            if (!G_inNodeList(adj->head, stackTop)) {
                G_rmEdge(adj->head, node);
            }
        }
    }

    //assign the remaining 'number' temporaries on the top of stack
    for (i = 0; i < number; i++) {
        //pop one node from top of stack
        G_node node = stackTop->head;
        stackTop = stackTop->tail;
        //copy available registers list
        Temp_tempList avail = Temp_deepCopy(regs);
        G_nodeList adj;
        for (adj = G_succ(node); adj; adj = adj->tail) {
            Temp_temp temp = (Temp_temp)Temp_look(ret.coloring, Live_gtemp(adj->head));
            avail = Temp_subtraction(avail, Temp_TempList(temp, NULL));
        }
        Temp_temp reg = avail->head;
        Temp_enter(ret.coloring, Live_gtemp(node), (string)reg);
    }

	return ret;
}
