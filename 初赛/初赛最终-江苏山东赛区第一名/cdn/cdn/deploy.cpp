#include "deploy.h"
#include <stdio.h>

#include "graph.h"
#include "output.h"
#include "greedy_search_alpha.h"
#include <ctime>
#include <fstream>

using namespace std;

//你要完成的功能总入口
void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{
    clock_t start = clock();
    char * topo_file = NULL;
    Graph* graph = new Graph(topo, line_num);
    GreedySearchAlpha* gsa = new GreedySearchAlpha(graph);
    //gsa->greedy_search_zero();
    gsa->topo_stratedy();
    //gsa->test_increase_server_by_pre_flow();
    vector<stack<int> > paths;
    gsa->mcf_machine->construct_solution(paths);

    construct_output(paths, topo_file, gsa->servers, gsa->global_min_cost);
    //construct_output(paths, topo_file);
    //write_result(topo_file, filename);


    //delete gsa;
    //delete graph;
    clock_t end = clock();
    printf("Use Time:%f\n",(double(end-start)/CLOCKS_PER_SEC));

	ofstream out_file("../record.txt", ios::app);
	if(!out_file.is_open())
	{
        cout<<"open file error!!!"<<endl;
        return;
	}
	out_file<<"case: "<<filename<<endl;
    out_file<<"server_fee: "<<graph->net_fee<<endl;
    out_file<<"node_num: "<<graph->net_node_num<<endl;
    out_file<<"customer_num: "<<graph->cus_node_num<<endl;
    out_file<<"server place:"<<endl;
    for(int i = 0; i < gsa->servers.size(); i++)
    {
        if(gsa->servers[i]) out_file<<i<<" ";
    }
    out_file<<endl;
    out_file<<"total cost: "<<gsa->global_min_cost<<endl;
    out_file<<"use time: "<<double(end-start)/CLOCKS_PER_SEC<<endl<<endl<<endl;

    out_file.close();

}
