#include "deploy.h"
#include <stdio.h>

#include "graph.h"
#include "min_cost_flow.h"
#include "ga.h"
#include "output.h"

#include "heuristic.h"

#include <ctime>

void test_GA(Graph* graph, char* &solution)
{
    GeneticAlgorithm* ga = new GeneticAlgorithm(graph);
    //ga->init_population();
    ga->evolve();
    cout << "min cost found: " << ga->best_ind.fitness << endl;
    // 构造输出
    ga->evaluate(ga->best_ind);
    if (ga->best_ind.feasible) cout << "feasible!" << endl;
    vector<stack<int> > paths;
    ga->god->construct_solution(paths);
    construct_output(paths, solution);
}

void test_heuristic(Graph *graph, char *&solution)
{
    HeuristicSolver *solver = new HeuristicSolver(graph);

    vector<stack<int> > paths;
    solver->solve(paths);

    construct_output(paths, solution);
}

//你要完成的功能总入口
void deploy_server(char * topo[MAX_EDGE_NUM], int line_num,char * filename)
{
    clock_t start = clock();
    char * topo_file = NULL;
    Graph* graph = new Graph(topo, line_num);
    //test_GA(graph, topo_file);
    test_heuristic(graph, topo_file);
	// 需要输出的内容
	//char * topo_file = (char *)"17\n\n0 8 0 20\n21 8 0 20\n9 11 1 13\n21 22 2 20\n23 22 2 8\n1 3 3 11\n24 3 3 17\n27 3 3 26\n24 3 3 10\n18 17 4 11\n1 19 5 26\n1 16 6 15\n15 13 7 13\n4 5 8 18\n2 25 9 15\n0 7 10 10\n23 24 11 23";

	// 直接调用输出文件的方法输出到指定文件中(ps请注意格式的正确性，如果有解，第一行只有一个数据；第二行为空；第三行开始才是具体的数据，数据之间用一个空格分隔开)
	write_result(topo_file, filename);
    clock_t end = clock();

    printf("Use Time:%f\n",(double(end-start)/CLOCKS_PER_SEC));

}
