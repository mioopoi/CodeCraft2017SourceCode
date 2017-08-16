#include "deploy.h"
#include <stdio.h>

#include "graph.h"
#include "output.h"
#include "greedy_search_alpha.h"
#include <ctime>

//你要完成的功能总入口
void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{
    clock_t start = clock();
    char * topo_file = NULL;
    Graph* graph = new Graph(topo, line_num);
    GreedySearchAlpha* gsa = new GreedySearchAlpha(graph);
    //gsa->greedy_search_zero();
    gsa->topo_stratedy();
    vector<stack<int> > paths;
    //gsa->mcf_machine->construct_solution(paths);
    construct_solution(paths);
    construct_output(paths, topo_file, gsa->servers, gsa->global_min_cost);
    //construct_output(paths, topo_file);

    //delete gsa;
    //delete graph;

	write_result(topo_file, filename);
	clock_t end = clock();

	printf("Use Time:%f\n",(double(end-start)/CLOCKS_PER_SEC));

}
