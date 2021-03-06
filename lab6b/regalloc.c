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
#include "liveness.h"
#include "regalloc.h"
#include "table.h"
#include "flowgraph.h"

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) {
	//your code here.
	struct RA_result ret;
    G_graph flowGraph = FG_AssemFlowGraph(il);
    struct Live_graph liveGraph = Live_liveness(flowGraph);
    struct COL_result color = COL_color(liveGraph.graph, F_tempMap, F_registers());
    ret.coloring = color.coloring;
    Temp_dumpMap(stdout, ret.coloring);
	return ret;
}
