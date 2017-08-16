#include "lib_io.h"
#include "graph.h"
#include "opt_search.h"
#include <stdio.h>
#include <fstream>

using namespace std;
//You need to complete the function

void construct_optput(vector<vector<int>>& paths, char* &flow_file)
{
    string res;
    res += to_string(paths.size());
    res += "\n\n";
    for (int i = 0; i < paths.size(); i++)
    {
        for (int j = 0; j < paths[i].size(); j++)
        {
            res += to_string(paths[i][j]);
            res += " ";
        }
        res += "\n";
    }

    res.pop_back();

    //cout << res << endl;
    flow_file = new char[res.length() + 1];
    strcpy(flow_file, res.c_str());
    //for (int i = 0; buffer[i]; ++i) printf("%c", buffer[i]);
}

void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{

	// Output demo
	clock_t start = clock();
	Graph* graph = new Graph(topo, line_num);
	OptSearch* opt_search = new OptSearch(graph);
	opt_search->topo_strategy();
    //opt_search->topo_strategy();

    vector<vector<int>> paths;
    char* flow_file;
    opt_search->mcf_simplex->construct_solution(paths, opt_search->servers);
    construct_optput(paths, flow_file);
	//char * topo_file = (char *)"17\n\n0 8 0 20\n21 8 0 20\n9 11 1 13\n21 22 2 20\n23 22 2 8\n1 3 3 11\n24 3 3 17\n27 3 3 26\n24 3 3 10\n18 17 4 11\n1 19 5 26\n1 16 6 15\n15 13 7 13\n4 5 8 18\n2 25 9 15\n0 7 10 10\n23 24 11 23";

	write_result(flow_file, filename);

    clock_t end = clock();
    printf("Use Time:%f\n",(double(end-start)/CLOCKS_PER_SEC));

    /*ofstream out_file("../record.txt", ios::app);
	if(!out_file.is_open())
	{
        cout<<"open file error!!!"<<endl;
        return;
	}
	out_file<<"case: "<<filename<<endl;
    out_file<<"node_num: "<<graph->node_num<<endl;
    out_file<<"customer_num: "<<graph->customer_num<<endl;
    out_file<<"server level: "<<endl;

    int cnt = 0;
    for(int i = 0; i < graph->node_num; i++)
    {
        if(opt_search->servers[i] != -1)
        {
            out_file<<"("<<i<<", "<<opt_search->servers[i]<<") ";
            cnt++;
        }

    }
    out_file<<endl<<"server num: "<<cnt<<endl;
    out_file<<"total cost: "<<opt_search->global_min_cost<<endl;
    out_file<<"use time: "<<double(end-start)/CLOCKS_PER_SEC<<endl<<endl<<endl;

    out_file.close();*/
}
