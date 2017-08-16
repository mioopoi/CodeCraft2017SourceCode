#ifndef __MCF_SIMPLEX_H__
#define __MCF_SIMPLEX_H__

#include <vector>
#include <iostream>

#include <ctime>

#include "net_simplex_mcf_helper.h"
#include "graph.h"

using namespace std;

int e_cap[MAX_EDGE_NUMBER]; // 有向边容量
int e_src[MAX_EDGE_NUMBER]; // 有向边起始点
int e_des[MAX_EDGE_NUMBER]; // 有向边指向点
int e_cost[MAX_EDGE_NUMBER]; // 有向边单位流量价格
int e_flow[MAX_EDGE_NUMBER]; // 有向边的流量
int n_deficit[MAX_NODE_NUMBER]; // 节点的需求/供应量

//Node all_nodes[MAX_NODE_NUMBER];
//Edge all_edges[MAX_EDGE_NUMBER];

//Candidate_T all_candidates[MAX_NODE_NUMBER];

int simplex_flow_array[MAX_NODE_NUMBER];
int simplex_cost_array[MAX_NODE_NUMBER];

int simplex_cost_matrix[2000][2000];
int simplex_flow_matrix[2000][2000];
int simplex_cost_matrix_cp[2000][2000];

//int mcf_cnt = 0;

//clock_t start_time, end_time;

class NetSimplexMCF
{
public:
	/* 单纯形相关 */
	void init_net();
	void create_init_base();
	void run_simplex();
	//Edge* rule_candidate_list_pivot();
	int rule_candidate_list_pivot();
	//inline void update_potential(int dt, Node *root);
	inline void update_potential(int dt, int root);
	inline void quick_sort_candidate_list(int min ,int max);
	//inline void swap_candidate(Candidate_T &candidate1 ,Candidate_T &candidate2);
	inline void swap_candidate(int candidate1, int candidate2);
	//inline int reduce_cost(Edge *edge);
	inline int reduce_cost(int edge);
	//inline Node* get_father(Node *node, Edge *edge);
	inline int get_father(int node, int edge);
	//Node* cut_and_update_tree(int dt, Node *root);
	int cut_and_update_tree(int dt, int root);
	/* 单纯形相关 */

	/* 接口函数 */
	NetSimplexMCF(Graph *graph);
	void init(int* servers, int level);
	bool solve();
	inline int get_min_cost();
	inline int get_max_flow();
	void set_edge_flow();
	inline int *get_edge_flow();
	inline vector<pair<int, int> > *get_server_flow();
	/* 接口函数 */

	Graph *graph; // 原图

	/*
	Node *nodes; // 节点
	Node *end_node;
	Node *virtual_root;

	Edge *edges; // 边
	Edge *end_edge;
	Edge *virtual_edges;
	Edge *end_virtual_edge;
	*/
	int nodes;
	int end_node;
	int virtual_root;

	int edges;
	int end_edge;
	int virtual_edges;
	int end_virtual_edge;

	int n; // 节点数
	int m; //边数

	//Candidate_T *candidates;
	int candidates;
	int n_candidate_lists;
	int tmp_candidate_list_sz;
	int real_candidate_list;
	int n_candidate_list;
	int hot_list_sz;

	int virtual_edge_max_cost;

	int status; // 求解状态
	int min_cost; // 最小费用
	int max_flow; // 最大流
	vector<pair<int, int> > server_flow; // 每台服务器服务的流量

public:
    int calculate_total_cost(int* servers, int update_type);
    void construct_solution(vector<vector<int>>& paths, int* servers);
    void calculate_server_out_flow(vector<Candidate*> candidates_cp);
    void calculate_node_pass_flow();
    void calculate_server_supply();
    void init_simplex_cost_matrix();
    int node_num;

};

NetSimplexMCF::NetSimplexMCF(Graph *graph)
{
    this->graph = graph;
    node_num = graph->node_num;

    for(int i = 0; i < this->node_num; i++)
    {
        memset(simplex_cost_matrix[i], 0, sizeof(int) * node_num);
        memset(simplex_flow_matrix[i], 0, sizeof(int) * node_num);
        memset(simplex_cost_matrix_cp[i], 0, sizeof(int) * node_num);
    }
}

void NetSimplexMCF::init(int *servers, int level)
{
	//start_time = clock();

	this->virtual_edge_max_cost = 101;
	server_flow.clear();
	this->n = this->graph->node_num + 1;
	this->m = this->graph->edge_num * 2;
	//this->m =

	// 因为要添加超源点到每台服务器的有向边，因此要更新边数
	for(int i = 0; i < this->graph->node_num; ++i) if(servers[i] != -1) this->m++;

	// 添加原图的边
	for(int i = 0; i < this->graph->edge_num * 2; ++i)
	{
		e_cap[i] = g_edge_cap[i];
		e_src[i] = g_edge_src[i];
		e_des[i] = g_edge_des[i];
		e_cost[i] = g_edge_cost[i];
	}

	// 添加超源点到服务器的边, 根据服务等级设置边的容量
	int power = g_server_level[this->graph->level_num - 1].power;
	int tmp_i = this->graph->edge_num * 2;
	for(int i = 0; i < this->graph->node_num; ++i)
	{
		if(servers[i] != -1)
		{
			if(level == 0) e_cap[tmp_i] = g_server_level[servers[i]].power;
			else e_cap[tmp_i] = power;
			e_src[tmp_i] = this->graph->node_num;
			e_des[tmp_i] = i;
			e_cost[tmp_i++] = 0;
		}
	}

	for(int i = 0; i < this->m; ++i) e_src[i]++, e_des[i]++;

	// 设置每个点的需求/供应量
	int sum_need = 0;
	for(int i = 0; i < this->graph->node_num; ++i)
	{
		if(g_node_customer[i] != -1)
		{
			n_deficit[i] = g_customer_need[g_node_customer[i]];
			sum_need += g_customer_need[g_node_customer[i]];
		}
		else n_deficit[i] = 0;
	}
	n_deficit[this->graph->node_num] = -sum_need;

	this->max_flow = sum_need;

	init_net();

	//end_time = clock();
	//printf("Use Time:%f\n",(double(end_time-start_time)/CLOCKS_PER_SEC));
	//start_time = clock();
}

void NetSimplexMCF::init_net()
{
	//this->nodes = all_nodes;
	//this->edges = all_edges;
	//this->virtual_edges = this->edges + this->m;
	this->nodes = this->edges = 0;
	this->virtual_edges = this->m;
	//this->candidates = all_candidates;
	this->candidates = 0;

	if(this->m < 8888)
	{
		this->hot_list_sz = 11;
		this->n_candidate_list = 55;
	}
	else
	{
		this->hot_list_sz = 22;
		this->n_candidate_list = 111;
	}

	//this->end_node = this->nodes + this->n;
	this->end_node = this->n;
	//Node *node = this->nodes;
	int node = 0;
	while(node != this->end_node)
	{
		//node->balance = n_deficit[node - this->nodes];
		_node_balance[node] = n_deficit[node];
		node++;
	}

	//this->end_edge = this->edges + this->m;
	this->end_edge = this->m;
	//Edge *edge = this->edges;
	int edge = 0;
	while(edge != this->end_edge)
	{
		/*int e_id = edge - this->edges;
		edge->id = e_id;
		edge->src = e_src[e_id] - 1;
		edge->des = e_des[e_id] - 1;
		edge->cost = e_cost[e_id];
		edge->cap = e_cap[e_id];
		edge->last = this->nodes + e_src[e_id] - 1;
		edge->first = this->nodes + e_des[e_id] - 1;
		edge++;*/
		_edge_src[edge] = e_src[edge] - 1;
		_edge_des[edge] = e_des[edge] - 1;
		_edge_cost[edge] = e_cost[edge];
		_edge_cap[edge] = e_cap[edge];
		_edge_last[edge] = e_src[edge] - 1;
		_edge_first[edge] = e_des[edge] - 1;
		edge++;
	}

	/*this->virtual_root = this->nodes + this->n;
	this->virtual_edges = this->edges + this->m;
	this->end_virtual_edge = this->virtual_edges + this->n;*/
	this->virtual_root = this->n;
	this->virtual_edges = this->m;
	this->end_virtual_edge = this->virtual_edges + this->n;

	this->status = UNSOLVED;
}

bool NetSimplexMCF::solve()
{
	//mcf_cnt++;

	create_init_base(); // 创建初始可行基

	run_simplex();

	// 找到最优解，记录min_cost和每台服务器服务的流量
	this->min_cost = 0;
	//Edge *edge = this->edges;
	int edge = 0;
	while(edge != this->end_edge)
	{
		//if(edge->ident == BASIC_EDGE || edge->ident == EDGE_UPPER) this->min_cost += edge->cost * edge->flow;
		//if(edge->src == this->graph->node_num) this->server_flow.push_back(make_pair(edge->des, edge->flow));
		if(_edge_ident[edge] == BASIC_EDGE || _edge_ident[edge] == EDGE_UPPER) this->min_cost += _edge_cost[edge] * _edge_flow[edge];
		if(_edge_src[edge] == this->graph->node_num) this->server_flow.push_back(make_pair(_edge_des[edge], _edge_flow[edge]));
		edge++;
	}
	edge = this->virtual_edges;
	while(edge != this->end_virtual_edge)
	{
		//if(edge->ident == BASIC_EDGE || edge->ident == EDGE_UPPER) this->min_cost += edge->cost * edge->flow;
		if(_edge_ident[edge] == BASIC_EDGE || _edge_ident[edge] == EDGE_UPPER) this->min_cost += _edge_cost[edge] * _edge_flow[edge];
		edge++;
	}

	//end_time = clock();
	//printf("Use Time:%f\n",(double(end_time-start_time)/CLOCKS_PER_SEC));
	//getchar();

	return this->status == OPTIMAL_SOLVED;
}

inline int NetSimplexMCF::get_min_cost() { return this->min_cost; }

inline int NetSimplexMCF::get_max_flow() { return this->max_flow; }

void NetSimplexMCF::set_edge_flow()
{
	int edge_num_origin = this->graph->edge_num * 2, cnt = 0;
	//Edge *edge = this->edges;
	int edge = 0;
	while(edge != this->end_edge && cnt < edge_num_origin)
	{
		/*int e_id = edge->id;
		if(edge->ident == BASIC_EDGE || edge->ident == EDGE_UPPER) e_flow[e_id] = edge->flow;
		else e_flow[e_id] = 0;*/
		if(_edge_ident[edge] == BASIC_EDGE || _edge_ident[edge] == EDGE_UPPER) e_flow[edge] = _edge_flow[edge];
		else e_flow[edge] = 0;
		cnt++, edge++;
	}
}

inline int* NetSimplexMCF::get_edge_flow() { return e_flow; }

inline vector<pair<int, int> >* NetSimplexMCF::get_server_flow() { return &(this->server_flow); }

void NetSimplexMCF::create_init_base()
{
	//Edge *edge = this->edges;
	//while(edge != this->end_edge) edge->flow = 0, edge->ident = EDGE_LOWER, edge++;
	int edge = 0;
	while(edge != this->end_edge) _edge_flow[edge] = 0, _edge_ident[edge] = EDGE_LOWER, edge++;

	edge = this->virtual_edges;
	while(edge != this->end_virtual_edge)
	{
		/*Node *node = this->nodes + (edge - this->virtual_edges);
		if(node->balance > 0) edge->flow = node->balance, edge->first = node, edge->last = this->virtual_root;
		else edge->flow = -node->balance, edge->first = this->virtual_root, edge->last = node;

		edge->cost = this->virtual_edge_max_cost, edge->ident = BASIC_EDGE, edge->cap = INF;*/
		int node = edge - this->virtual_edges;
		if(_node_balance[node] > 0) _edge_flow[edge] = _node_balance[node], _edge_first[edge] = node, _edge_last[edge] = this->virtual_root;
		else _edge_flow[edge] = -_node_balance[node], _edge_first[edge] = this->virtual_root, _edge_last[edge] = node;

		_edge_cost[edge] = this->virtual_edge_max_cost, _edge_ident[edge] = BASIC_EDGE, _edge_cap[edge] = INF;
		edge++;
	}

	/*
	this->virtual_root->balance = 0;
	this->virtual_root->potential = this->virtual_edge_max_cost;
	this->virtual_root->sub_tree_lv = 0;
	this->virtual_root->entering_edge = NULL;
	this->virtual_root->prev = NULL;
	this->virtual_root->next = this->nodes;
	*/
	_node_balance[this->virtual_root] = 0;
	_node_potential[this->virtual_root] = this->virtual_edge_max_cost;
	_node_sub_tree_lv[this->virtual_root] = 0;
	_node_entering_edge[this->virtual_root] = -1;
	_node_prev[this->virtual_root] = -1;
	_node_next[this->virtual_root] = 0;

	//Node *node = this->nodes;
	int node = 0;
	while(node != this->end_node)
	{
		/*node->potential = node->balance > 0 ? this->virtual_edge_max_cost * 2 : 0;
		node->sub_tree_lv = 1;
		node->entering_edge = this->virtual_edges + (node - this->nodes);
		node->prev = node - 1;
		node->next = node + 1;*/
		_node_potential[node] = _node_balance[node] > 0 ? this->virtual_edge_max_cost * 2 : 0;
		_node_sub_tree_lv[node] = 1;
		_node_entering_edge[node] = this->virtual_edges + node;
		_node_prev[node] = node - 1;
		_node_next[node] = node + 1;
		node++;
	}

	//(this->nodes + n - 1)->next = NULL;
	//this->nodes->prev = this->virtual_root;
	_node_next[n-1] = -1;
	_node_prev[0] = this->virtual_root;
}

void NetSimplexMCF::run_simplex()
{
	//Edge *entering_edge, *leaving_edge;
	int entering_edge, leaving_edge;

	this->n_candidate_lists = ((this->m - 1) / this->n_candidate_list) + 1;
	this->real_candidate_list = this->tmp_candidate_list_sz = 0;

	while(true)
	{
		if(this->status != UNSOLVED) break;

		entering_edge = rule_candidate_list_pivot();

		//if(entering_edge)
		if(entering_edge != -1)
		{
			//Edge *edge;
			//Node *k1, *k2;
			int edge, k1, k2;
			int t, alpha;

			//if(entering_edge->ident == EDGE_UPPER) k1 = entering_edge->first, k2 = entering_edge->last, alpha = entering_edge->flow;
			//else k1 = entering_edge->last, k2 = entering_edge->first, alpha = entering_edge->cap - entering_edge->flow;
			if(_edge_ident[entering_edge] == EDGE_UPPER) k1 = _edge_first[entering_edge], k2 = _edge_last[entering_edge], alpha = _edge_flow[entering_edge];
			else k1 = _edge_last[entering_edge], k2 = _edge_first[entering_edge], alpha = _edge_cap[entering_edge] - _edge_flow[entering_edge];

			//Node *mem_k1 = k1, *mem_k2 = k2;
			//leaving_edge = NULL;
			int mem_k1 = k1, mem_k2 = k2;
			leaving_edge = -1;
			bool leave, leaving_reduces_flow = reduce_cost(entering_edge) > 0;
			while(true)
			{
				if(k1 == k2) break;

				//if(k1->sub_tree_lv <= k2->sub_tree_lv)
				if(_node_sub_tree_lv[k1] <= _node_sub_tree_lv[k2])
				{
					//edge = k2->entering_edge;
					//if(edge->last != k2) t = edge->flow, leave = true;
					//else t = edge->cap - edge->flow, leave = false;
					edge = _node_entering_edge[k2];
					if(_edge_last[edge] != k2) t = _edge_flow[edge], leave = true;
					else t = _edge_cap[edge] - _edge_flow[edge], leave = false;

					if(t <= alpha) alpha = t, leaving_edge = edge, leaving_reduces_flow = leave;

					k2 = get_father(k2, edge);
				}
				else
				{
					//edge = k1->entering_edge;
					//if(edge->last != k1) t = edge->cap - edge->flow, leave = false;
					//else t = edge->flow, leave = true;
					edge = _node_entering_edge[k1];
					if(_edge_last[edge] != k1) t = _edge_cap[edge] - _edge_flow[edge], leave = false;
					else t = _edge_flow[edge], leave = true;

					if(alpha > t) leaving_reduces_flow = leave, leaving_edge = edge, alpha = t;

					k1 = get_father(k1 , edge);
				}
			}

			//if(!leaving_edge) leaving_edge = entering_edge;
			if(leaving_edge == -1) leaving_edge = entering_edge;
			if(alpha >= INF) { this->status = UNBOUNDED; break; }

			k1 = mem_k1; k2 = mem_k2;
			if(alpha)
			{
				//entering_edge->flow += entering_edge->last == k1 ? alpha : -alpha;
				_edge_flow[entering_edge] += _edge_last[entering_edge] == k1 ? alpha : -alpha;
				while(true)
				{
					if(k1 == k2) break;

					//if(k1->sub_tree_lv <= k2->sub_tree_lv)
					if(_node_sub_tree_lv[k1] <= _node_sub_tree_lv[k2])
					{
						//edge = k2->entering_edge;
						//edge->flow += edge->last == k2 ? alpha : -alpha;
						//k2 = get_father(k2, k2->entering_edge);
						edge = _node_entering_edge[k2];
						_edge_flow[edge] += _edge_last[edge] == k2 ? alpha : -alpha;
						k2 = get_father(k2, _node_entering_edge[k2]);
					}
					else
					{
						//edge = k1->entering_edge;
						//edge->flow += edge->last != k1 ? alpha : -alpha;
						//k1 = get_father(k1, k1->entering_edge);
						edge = _node_entering_edge[k1];
						_edge_flow[edge] += _edge_last[edge] != k1 ? alpha : -alpha;
						k1 = get_father(k1, _node_entering_edge[k1]);
					}
				}
			}

			if(entering_edge != leaving_edge)
			{
				bool leaving_bring_flow_in_T2 = true;
				//if(leaving_edge->last->sub_tree_lv > leaving_edge->first->sub_tree_lv && !leaving_reduces_flow)
				if(_node_sub_tree_lv[_edge_last[leaving_edge]] > _node_sub_tree_lv[_edge_first[leaving_edge]] && !leaving_reduces_flow)
				{
					leaving_bring_flow_in_T2 = false;
				}
				//if(leaving_edge->last->sub_tree_lv <= leaving_edge->first->sub_tree_lv && leaving_reduces_flow)
				if(_node_sub_tree_lv[_edge_last[leaving_edge]] <= _node_sub_tree_lv[_edge_first[leaving_edge]] && leaving_reduces_flow)
				{
					leaving_bring_flow_in_T2 = false;
				}
				//if(leaving_bring_flow_in_T2 != (mem_k1 == entering_edge->last))
				if(leaving_bring_flow_in_T2 != (mem_k1 == _edge_last[entering_edge]))
				{
					//k2 = entering_edge->first, k1 = entering_edge->last;
					k2 = _edge_first[entering_edge], k1 = _edge_last[entering_edge];
				}
				else
				{
					//k2 = entering_edge->last, k1 = entering_edge->first;
					k2 = _edge_last[entering_edge], k1 = _edge_first[entering_edge];
				}
			}

			//leaving_edge->ident = leaving_reduces_flow ? EDGE_LOWER : EDGE_UPPER;
			_edge_ident[leaving_edge] = leaving_reduces_flow ? EDGE_LOWER : EDGE_UPPER;

			if (leaving_edge != entering_edge)
			{
				//entering_edge->ident = BASIC_EDGE;
				_edge_ident[entering_edge] = BASIC_EDGE;
				//Node *h1, *h2;
				int h1, h2;
				//if(leaving_edge->last->sub_tree_lv >= leaving_edge->first->sub_tree_lv)
				if(_node_sub_tree_lv[_edge_last[leaving_edge]] >= _node_sub_tree_lv[_edge_first[leaving_edge]])
				{
					//h1 = leaving_edge->first, h2 = leaving_edge->last;
					h1 = _edge_first[leaving_edge], h2 = _edge_last[leaving_edge];
				}
				else
				{
					//h1 = leaving_edge->last, h2 = leaving_edge->first;
					h1 = _edge_last[leaving_edge], h2 = _edge_first[leaving_edge];
				}

				//Node *root = k2, *father, *prev_node = k1, *last_node;
				//Edge *edge1 = entering_edge, *edge2;
				//int dt = (k1->sub_tree_lv) + 1 - (k2->sub_tree_lv);
				int root = k2, father, prev_node = k1, last_node;
				int edge1 = entering_edge, edge2;
				int dt = (_node_sub_tree_lv[k1]) + 1 - (_node_sub_tree_lv[k2]);

				bool flag = false;
				while(!flag)
				{
					flag = (root == h2);

					/*father = get_father(root ,root->entering_edge);
					last_node = cut_and_update_tree(dt, root);
					root->prev = prev_node;
					last_node->next = NULL;
					Node *tmp = prev_node->next;
					if(tmp) last_node->next = tmp, tmp->prev = last_node;
					prev_node->next = root;*/
					father = get_father(root, _node_entering_edge[root]);
					last_node = cut_and_update_tree(dt, root);
					_node_prev[root] = prev_node;
					_node_next[last_node] = -1;
					int tmp = _node_next[prev_node];
					if(tmp != -1) _node_next[last_node] = tmp, _node_prev[tmp] = last_node;
					_node_next[prev_node] = root;
					prev_node = last_node;
					dt += 2;

					//edge2 = root->entering_edge;
					//root->entering_edge = edge1;
					edge2 = _node_entering_edge[root];
					_node_entering_edge[root] = edge1;
					edge1 = edge2;
					root = father;
				}

				//k2 = entering_edge->first;
				k2 = _edge_first[entering_edge];
				dt = reduce_cost(entering_edge);
				//if(entering_edge->first->sub_tree_lv < entering_edge->last->sub_tree_lv)
				if(_node_sub_tree_lv[_edge_first[entering_edge]] < _node_sub_tree_lv[_edge_last[entering_edge]])
				{
					//dt = -dt, k2 = entering_edge->last;
					dt = -dt, k2 = _edge_last[entering_edge];
				}

				update_potential(dt, k2);
			}
		}
		else
		{
			this->status = OPTIMAL_SOLVED;
			//Edge *edge = this->virtual_edges;
			int edge = this->virtual_edges;
			while(edge != this->end_virtual_edge)
			{
				//if(edge->flow > 0) this->status = UNFEASIBLE;
				if(_edge_flow[edge] > 0) this->status = UNFEASIBLE;
				edge++;
			}
		}
	}
}

//Edge * NetSimplexMCF::rule_candidate_list_pivot()
int NetSimplexMCF::rule_candidate_list_pivot()
{
	int min_val = this->hot_list_sz < this->tmp_candidate_list_sz ? this->hot_list_sz : this->tmp_candidate_list_sz;

	int next = 0, i = 2;
	while(true)
	{
		if(i > min_val) break;

		//Edge *edge = this->candidates[i].edge;
		int edge = candidate_edge[i];
		int red_cost = reduce_cost(edge);

		//if((red_cost < 0 && edge->ident == EDGE_LOWER) || (red_cost > 0 && edge->ident == EDGE_UPPER))
		if((red_cost < 0 && _edge_ident[edge] == EDGE_LOWER) || (red_cost > 0 && _edge_ident[edge] == EDGE_UPPER))
		{
			next++;
			//this->candidates[ next ].edge = edge;
			//this->candidates[ next ].abs_rc = red_cost >=0 ? red_cost : -red_cost;
			candidate_edge[next] = edge;
			candidate_abs_rc[next] = red_cost >=0 ? red_cost : -red_cost;
		}
		i++;
	}

	this->tmp_candidate_list_sz = next;
	int old_candidate_list = this->real_candidate_list;
	while(true)
	{
		//Edge *edge = this->edges + this->real_candidate_list;
		int edge = this->real_candidate_list;
		while(edge < this->end_edge)
		{
			int red_cost = reduce_cost(edge);
			//if(red_cost > 0 && edge->ident == EDGE_UPPER)
			if(red_cost > 0 && _edge_ident[edge] == EDGE_UPPER)
			{
				//this->candidates[++this->tmp_candidate_list_sz].edge = edge;
				//this->candidates[this->tmp_candidate_list_sz].abs_rc = red_cost >= 0 ? red_cost : -red_cost;
				candidate_edge[++this->tmp_candidate_list_sz] = edge;
				candidate_abs_rc[this->tmp_candidate_list_sz] = red_cost >= 0 ? red_cost : -red_cost;
			}
			//if(red_cost < 0 && edge->ident == EDGE_LOWER)
			if(red_cost < 0 && _edge_ident[edge] == EDGE_LOWER)
			{
				//this->candidates[++this->tmp_candidate_list_sz].edge = edge;
				//this->candidates[this->tmp_candidate_list_sz].abs_rc = red_cost >= 0 ? red_cost : -red_cost;
				candidate_edge[++this->tmp_candidate_list_sz] = edge;
				candidate_abs_rc[this->tmp_candidate_list_sz] = red_cost >= 0 ? red_cost : -red_cost;
			}
			edge += this->n_candidate_lists;
		}

		this->real_candidate_list++;
		if(this->real_candidate_list == this->n_candidate_lists) this->real_candidate_list = 0;

		if(this->tmp_candidate_list_sz >= this->hot_list_sz || this->real_candidate_list == old_candidate_list) break;
	}

	if(this->tmp_candidate_list_sz)
	{
		quick_sort_candidate_list(1 , this->tmp_candidate_list_sz);
		//return this->candidates[ 1 ].edge;
		return candidate_edge[1];
	}
	//return NULL;
	return -1;
}

//inline void NetSimplexMCF::update_potential(int dt, Node *root)
inline void NetSimplexMCF::update_potential(int dt, int root)
{
	//Node *node = root;
	int node = root;
	//int level = root->sub_tree_lv;
	int level = _node_sub_tree_lv[root];

	while(true)
	{
		//node->potential += dt;
		//node = node->next;
		//if(!node || node->sub_tree_lv <= level) break;
		_node_potential[node] += dt;
		node = _node_next[node];
		if(node == -1 || _node_sub_tree_lv[node] <= level) break;
	}
}

inline void NetSimplexMCF::quick_sort_candidate_list(int min ,int max)
{
	int left = min, right = max;
	//int cut = this->candidates[(left + right) >> 1].abs_rc;
	int cut = candidate_abs_rc[(left+right) >> 1];
	while(true)
	{
		//while(this->candidates[left].abs_rc > cut) left++;
		//while(cut > this->candidates[right].abs_rc) right--;
		while(candidate_abs_rc[left] > cut) left++;
		while(cut > candidate_abs_rc[right]) right--;

		//if(left < right) swap_candidate(this->candidates[left] , this->candidates[right]);
		if(left < right) swap_candidate(left, right);
		if(left <= right) left++, right--;

		if(left > right) break;
	}

	if(min < right) quick_sort_candidate_list(min ,right);
	if(left < max && left <= this->hot_list_sz) quick_sort_candidate_list(left ,max);
}

//inline void NetSimplexMCF::swap_candidate(Candidate_T &candidate1 ,Candidate_T &candidate2)
inline void NetSimplexMCF::swap_candidate(int candidate1, int candidate2)
{
	//Candidate_T tmp = candidate1;
	//candidate1 = candidate2;
	//candidate2 = tmp;
	int tmp = candidate_edge[candidate1];
	candidate_edge[candidate1] = candidate_edge[candidate2];
	candidate_edge[candidate2] = tmp;
	tmp = candidate_abs_rc[candidate1];
	candidate_abs_rc[candidate1] = candidate_abs_rc[candidate2];
	candidate_abs_rc[candidate2] = tmp;
}

//inline int NetSimplexMCF::reduce_cost(Edge *edge)
inline int NetSimplexMCF::reduce_cost(int edge)
{
	//int red_cost = (edge->last)->potential - (edge->first)->potential;
	//return red_cost + edge->cost;
	int red_cost = _node_potential[_edge_last[edge]] - _node_potential[_edge_first[edge]];
	return red_cost + _edge_cost[edge];
}

//inline Node* NetSimplexMCF::get_father(Node *node ,Edge *edge)
inline int NetSimplexMCF::get_father(int node, int edge)
{
	//if(!edge) return NULL;
	//return edge->last == node ? edge->first : edge->last;
	if(edge == -1) return -1;
	return _edge_last[edge] == node ? _edge_first[edge] : _edge_last[edge];
}

//Node* NetSimplexMCF::cut_and_update_tree(int dt, Node *root)
int NetSimplexMCF::cut_and_update_tree(int dt, int root)
{
	//Node *node = root;
	//int level = root->sub_tree_lv;
	int node = root;
	int level = _node_sub_tree_lv[root];

	while(true)
	{
		//if(!node->next || node->next->sub_tree_lv <= level) break;
		//node = node->next;
		//node->sub_tree_lv += dt;
		if(_node_next[node] == -1 || _node_sub_tree_lv[_node_next[node]] <= level) break;
		node = _node_next[node];
		_node_sub_tree_lv[node] += dt;
	}
	//root->sub_tree_lv += dt;
	_node_sub_tree_lv[root] += dt;

	//Node *tmp1 = root->prev, *tmp2 = node->next;
	int tmp1 = _node_prev[root], tmp2 = _node_next[node];
	//if(NULL != tmp1) tmp1->next = tmp2;
	//if(NULL != tmp2) tmp2->prev = tmp1;
	if(-1 != tmp1) _node_next[tmp1] = tmp2;
	if(-1 != tmp2) _node_prev[tmp2] = tmp1;

	return node;
}

//run_type 0 按流量更新服务器和费用
//run_type == 1 按服务器等级跑, 按流量更新服务器和费用
int NetSimplexMCF::calculate_total_cost(int* servers, int run_type)
{
    int server_id, flow, level, total_cost = min_cost;
    int cnt1 = 0;

    if(run_type == 0)
    {
        for(int i = 0; i < server_flow.size(); i++)
        {
            server_id = server_flow[i].first;
            flow = server_flow[i].second;
            cnt1 += flow;
            level = g_server_level_index[flow];
            //cout<<"server: "<<flow<<" "<<server_id<<" "<<level<<endl;
            if(flow == 0) continue;
            total_cost += g_node_cost[server_id] + g_server_level[level].cost;
            servers[server_id] = level;
                //total_cost += g_node_cost[server_id] + g_server_level[servers[server_id]].cost;
        }
    }
    else if(run_type == 1)
    {
        for(int i = 0; i < graph->node_num; i++)
        {
            if(servers[i] == -1) continue;
            server_id = i;
            total_cost += g_node_cost[server_id] + g_server_level[servers[server_id]].cost;
        }
    }
    return total_cost;
}

void NetSimplexMCF::construct_solution(vector<vector<int>>& paths, int* servers)
{
    set_edge_flow();

    vector<int> edge_path(graph->node_num + 4, -1);
    int flow_cnt = 0, last;

    int index = 0;
    int cur, cur_flow, out_flow, server_id, cnt;

    int cnt1 = 0;
    while(index != server_flow.size())
    {
        cnt1 += server_flow[index].second;
        server_id = server_flow[index].first;
        out_flow = server_flow[index].second;
        while(out_flow != 0)
        {
            cur = server_id;
            cur_flow = INF;
            cnt = 0;
            while(true)
            {
                int i;
                for(i = g_node_head[cur]; i != -1; i = g_edge_next[i])
                {
                    if(e_flow[i] <= 0) continue;
                    if(e_flow[i] < cur_flow) cur_flow = e_flow[i];
                    edge_path[cnt++] = i;
                    cur = g_edge_des[i];
                    break;
                }
                if(i == -1) break;
            }

            if(out_flow <=  cur_flow) cur_flow = out_flow;
            if(cur == server_id)
            {
                cur_flow = out_flow;
                vector<int> path(4, 0);
                path[0] = server_id;
                path[1] = g_node_customer[server_id];
                path[2] = cur_flow;
                path[3] = servers[server_id];
                paths.push_back(path);
            }
            else
            {
                vector<int> path(cnt + 4, 0);
                path[0] = server_id;
                for(int i = 0; i < cnt; i++)
                {
                    e_flow[edge_path[i]] -= cur_flow;
                    path[i+1] = g_edge_des[edge_path[i]];
                }
                last = path[cnt];
                path[cnt + 3] = servers[server_id];
                path[cnt + 2] = cur_flow;
                path[cnt + 1] = g_node_customer[last];
                paths.push_back(path);
            }

            flow_cnt += cur_flow;
            out_flow -= cur_flow;
        }
        index++;
    }

    cout<<"flow cnt: "<<flow_cnt<<" "<<cnt1<<endl;
}

void NetSimplexMCF::calculate_server_out_flow(vector<Candidate*> candidates_cp)
{
    int server_id, flow, level, total_cost = min_cost;
    int cnt1 = 0;
    for(int i = 0; i < server_flow.size(); i++)
    {
        server_id = server_flow[i].first;
        flow = server_flow[i].second;
        candidates_cp[server_id]->server_out_flow = flow;
    }
}

void NetSimplexMCF::calculate_node_pass_flow()
{
    memset(simplex_flow_array, 0, sizeof(int) * node_num);
    memset(simplex_cost_array, 0, sizeof(int) * node_num);
    set_edge_flow();

    vector<int> edge_path(graph->node_num + 4, -1);
    int flow_cnt = 0, last;

    int index = 0;
    int cur, cur_flow, out_flow, server_id, cnt;

    int cnt1 = 0;
    while(index != server_flow.size())
    {
        cnt1 += server_flow[index].second;
        server_id = server_flow[index].first;
        out_flow = server_flow[index].second;
        while(out_flow != 0)
        {
            cur = server_id;
            cur_flow = INF;
            cnt = 0;
            while(true)
            {
                int i;
                for(i = g_node_head[cur]; i != -1; i = g_edge_next[i])
                {
                    if(e_flow[i] <= 0) continue;
                    if(e_flow[i] < cur_flow) cur_flow = e_flow[i];
                    edge_path[cnt++] = i;
                    cur = g_edge_des[i];
                    break;
                }
                if(i == -1) break;
            }

            if(out_flow <=  cur_flow) cur_flow = out_flow;
            if(cur == server_id)
            {
                cur_flow = out_flow;
            }
            else
            {
                int des, cost = 0;
                for(int i = 0; i < cnt; i++)
                {
                    e_flow[edge_path[i]] -= cur_flow;
                    des = g_edge_des[edge_path[i]];
                    cost += cur_flow * e_des[edge_path[i]];
                    simplex_flow_array[des] += cur_flow;
                    simplex_cost_array[des] += cur_flow * e_cost[edge_path[i]];
                }

            }

            flow_cnt += cur_flow;
            out_flow -= cur_flow;
        }
        index++;
    }

    cout<<"node pass flow cnt: "<<flow_cnt<<" "<<cnt1<<endl;
}

void NetSimplexMCF::calculate_server_supply()
{
    set_edge_flow();

    vector<int> edge_path(graph->node_num + 4, -1);
    int flow_cnt = 0, last;

    int index = 0;
    int cur, cur_flow, out_flow, server_id, cnt;

    int cnt1 = 0, total_cost = 0;
    while(index != server_flow.size())
    {
        cnt1 += server_flow[index].second;
        server_id = server_flow[index].first;
        out_flow = server_flow[index].second;
        while(out_flow != 0)
        {
            cur = server_id;
            cur_flow = INF;
            cnt = 0;
            while(true)
            {
                int i;
                for(i = g_node_head[cur]; i != -1; i = g_edge_next[i])
                {
                    if(e_flow[i] <= 0) continue;
                    if(e_flow[i] < cur_flow) cur_flow = e_flow[i];
                    edge_path[cnt++] = i;
                    cur = g_edge_des[i];
                    break;
                }
                if(i == -1) break;
            }

            if(out_flow <=  cur_flow) cur_flow = out_flow;
            if(cur == server_id)
            {
                cur_flow = out_flow;
                simplex_flow_matrix[server_id][server_id] += cur_flow;
            }
            else
            {
                int des, cost = 0;
                for(int i = 0; i < cnt; i++)
                {
                    e_flow[edge_path[i]] -= cur_flow;
                    des = g_edge_des[edge_path[i]];
                    cost += e_cost[edge_path[i]] * cur_flow;
                    total_cost += cost;
                }
                simplex_flow_matrix[server_id][des] += cur_flow;
                simplex_cost_matrix[server_id][des] += cost;
            }

            flow_cnt += cur_flow;
            out_flow -= cur_flow;
        }
        index++;
    }

    //cout<<"node pass flow cnt: "<<flow_cnt<<"  cost cnt: "<<total_cost<<endl;
}

void NetSimplexMCF::init_simplex_cost_matrix()
{
    for(int i = 0; i < this->node_num; i++)
    {
        memset(simplex_cost_matrix[i], 0, sizeof(int) * node_num);
        memset(simplex_flow_matrix[i], 0, sizeof(int) * node_num);
    }
}

#endif
