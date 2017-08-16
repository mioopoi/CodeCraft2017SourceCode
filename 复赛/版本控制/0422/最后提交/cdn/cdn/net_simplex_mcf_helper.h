#ifndef __MCF_SIMPLEX_HELPER_H__
#define __MCF_SIMPLEX_HELPER_H__

const int BASIC_EDGE = 0;
const int EDGE_LOWER = 1;
const int EDGE_UPPER = 2;

const int N_CANDIDATE_LIST_L = 55;
const int N_CANDIDATE_LIST_M = 111;
const int N_CANDIDATE_LIST_H = 333;
const int SIZE_HOT_LIST_L = 11;
const int SIZE_HOT_LIST_M = 22;
const int SIZE_HOT_LIST_H = 66;

const int UNSOLVED = 0;
const int OPTIMAL_SOLVED = 1;
const int UNFEASIBLE = 2;
const int UNBOUNDED = 3;

struct Edge;

struct Node
{
	int balance, potential, sub_tree_lv;
	struct Edge *entering_edge;
	struct Node *prev, *next;
};

struct Edge
{
	int id;
	int src, des, cap, flow, cost, ident;
	struct Node *first, *last;
};

struct Candidate_T
{
	struct Edge *edge;
	int abs_rc;
};

#endif