#ifndef __MCF_SIMPLEX_HELPER_H__
#define __MCF_SIMPLEX_HELPER_H__

#include "graph.h"

const int BASIC_EDGE = 0;
const int EDGE_LOWER = 1;
const int EDGE_UPPER = 2;

const int UNSOLVED = 0;
const int OPTIMAL_SOLVED = 1;
const int UNFEASIBLE = 2;
const int UNBOUNDED = 3;

/*
struct Edge;

struct Node
{
	int balance, potential, sub_tree_lv;
	struct Edge *entering_edge;
	struct Node *prev, *next;
};

struct Edge
{
	int id, src, des, cap, flow, cost, ident;
	struct Node *first, *last;
};

struct Candidate_T
{
	struct Edge *edge;
	int abs_rc;
};

struct Candidate_T
{
	int edge;
	int abs_rc;
};
*/

int _node_balance[MAX_NODE_NUMBER];
int _node_potential[MAX_NODE_NUMBER];
int _node_sub_tree_lv[MAX_NODE_NUMBER];
int _node_entering_edge[MAX_NODE_NUMBER];
int _node_prev[MAX_NODE_NUMBER];
int _node_next[MAX_NODE_NUMBER];

int _edge_src[MAX_EDGE_NUMBER];
int _edge_des[MAX_EDGE_NUMBER];
int _edge_cap[MAX_EDGE_NUMBER];
int _edge_flow[MAX_EDGE_NUMBER];
int _edge_cost[MAX_EDGE_NUMBER];
int _edge_ident[MAX_EDGE_NUMBER];
int _edge_first[MAX_EDGE_NUMBER];
int _edge_last[MAX_EDGE_NUMBER];

int candidate_edge[MAX_EDGE_NUMBER];
int candidate_abs_rc[MAX_EDGE_NUMBER];


#endif