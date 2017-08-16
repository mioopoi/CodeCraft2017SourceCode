#ifndef _HEURISTIC_H_
#define _HEURISTIC_H_

#include <iostream>
#include <vector>
#include <stack>

#include "globals.h"
#include "graph.h"
#include "mcf.h"

using namespace std;


class HeuristicSolver
{
public:
	Graph *graph;
	MCF *mcf;

	vector<Cus_Node *> cus_nodes;

	HeuristicSolver(Graph *graph);

	void solve(vector<stack<int> > &paths);

private:
	//
};

HeuristicSolver::HeuristicSolver(Graph *graph)
{
	this->graph = graph;
	this->mcf = new MCF(graph);

	for(Cus_Node *node : graph->cus_nodes)
	{
		this->cus_nodes.push_back(new Cus_Node(node->src, node->des, node->need));
	}

}

void HeuristicSolver::solve(vector<stack<int> > &paths)
{
	while(this->mcf->compute(cus_nodes));

	this->mcf->construct_solution(cus_nodes, paths);
}

#endif