#ifndef __MCF_SIMPLEX_H__
#define __MCF_SIMPLEX_H__

#include <vector>
#include <iostream>

#include "net_simplex_mcf_helper.h"
#include "graph.h"

using namespace std;

int e_cap[MAX_EDGE_NUMBER]; // 有向边容量
int e_src[MAX_EDGE_NUMBER]; // 有向边起始点
int e_des[MAX_EDGE_NUMBER]; // 有向边指向点
int e_cost[MAX_EDGE_NUMBER]; // 有向边单位流量价格
int e_flow[MAX_EDGE_NUMBER]; // 有向边的流量
int n_deficit[MAX_NODE_NUMBER]; // 节点的需求/供应量

Node all_nodes[MAX_NODE_NUMBER];
Edge all_edges[MAX_EDGE_NUMBER];

Candidate_T all_candidates[MAX_NODE_NUMBER];

int simplex_flow_array[MAX_NODE_NUMBER];
int simplex_cost_array[MAX_NODE_NUMBER];

int simplex_cost_matrix[2000][2000];
int simplex_flow_matrix[2000][2000];
int simplex_cost_matrix_cp[2000][2000];

class NetSimplexMCF
{
public:
	/* 单纯形相关 */
	void init_net();
	void create_init_base();
	void run_simplex();
	Edge* rule_candidate_list_pivot();
	void update_T(Node *h1, Node *h2, Node *k1, Node *k2, Edge *h, Edge *k);
	inline void update_potential(int dt, Node *root);
	inline void quick_sort_candidate_list(int min ,int max);
	inline void swap_candidate(Candidate_T &candidate1 ,Candidate_T &candidate2);
	inline int reduce_cost(Edge *edge);
	inline Node* get_father(Node *node, Edge *edge);
	Node* cut_and_update_sub_tree(int dt, Node *root);
	void paste_sub_tree(Node *last_node ,Node *prev_node, Node *root);
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
	int n; // 节点数
	int m; //边数

	int status; // 求解状态

	Node *nodes; // 节点
	Node *end_node;
	Node *virtual_root;

	Edge *edges; // 边
	Edge *end_edge;
	Edge *virtual_edges;
	Edge *end_virtual_edge;

	Candidate_T *candidates;
	int n_candidate_lists;
	int tmp_candidate_list_sz;
	int real_candidate_list;
	int n_candidate_list;
	int hot_list_sz;

	int virtual_edge_max_cost;

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
	this->virtual_edge_max_cost = 1111111;
	server_flow.clear();
	this->n = this->graph->node_num + 1;
	this->m = this->graph->edge_num * 2;

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
}

void NetSimplexMCF::init_net()
{
	this->nodes = all_nodes;
	this->edges = all_edges;
	this->virtual_edges = this->edges + this->m;

	if(this->m < 11111)
	{
		this->n_candidate_list = N_CANDIDATE_LIST_L;
		this->hot_list_sz = SIZE_HOT_LIST_L;
	}
	else if(this->m < 111111)
	{
		this->n_candidate_list = N_CANDIDATE_LIST_M;
		this->hot_list_sz = SIZE_HOT_LIST_M;
	}
	else
	{
		this->n_candidate_list = N_CANDIDATE_LIST_H;
		this->hot_list_sz = SIZE_HOT_LIST_H;
	}
	this->candidates = all_candidates;

	this->end_node = this->nodes + this->n;
	for(Node *node = this->nodes; node != this->end_node; node++ )
	{
		node->balance = n_deficit[node - this->nodes];
	}

	this->end_edge = this->edges + this->m;
	for(Edge *edge = this->edges; edge != this->end_edge; edge++ )
	{
		int e_id = edge - this->edges;
		edge->id = e_id;
		edge->src = e_src[e_id] - 1;
		edge->des = e_des[e_id] - 1;
		edge->cost = e_cost[e_id];
		edge->cap = e_cap[e_id];
		edge->last = this->nodes + e_src[e_id] - 1;
		edge->first = this->nodes + e_des[e_id] - 1;
	}

	this->virtual_root = this->nodes + this->n;
	this->virtual_edges = this->edges + this->m;
	this->end_virtual_edge = this->virtual_edges + this->n;

	this->status = UNSOLVED;
}

bool NetSimplexMCF::solve()
{
	create_init_base(); // 创建初始可行基

	run_simplex();

	// 找到最优解，记录min_cost和每台服务器服务的流量
	this->min_cost = 0;
	for(Edge *edge = this->edges; edge != this->end_edge; edge++)
	{
		if(edge->ident == BASIC_EDGE || edge->ident == EDGE_UPPER)
		{
			this->min_cost += edge->cost * edge->flow;
		}
		if(edge->src == this->graph->node_num)
		{
			//cout<<"edge->src = "<<edge->src<<", edge->des = "<<edge->des<<", edge->flow = "<<edge->flow<<endl;
			this->server_flow.push_back(make_pair(edge->des, edge->flow));
		}
	}
	for(Edge *edge = this->virtual_edges; edge != this->end_virtual_edge; edge++)
	{
		if(edge->ident == BASIC_EDGE || edge->ident == EDGE_UPPER)
		{
			this->min_cost += edge->cost * edge->flow;
		}
	}

    	// 如果没找到最优解，返回false
	if(this->status != OPTIMAL_SOLVED) return false;
	return true;
}

inline int NetSimplexMCF::get_min_cost() { return this->min_cost; }

inline int NetSimplexMCF::get_max_flow() { return this->max_flow; }

void NetSimplexMCF::set_edge_flow()
{
	int edge_num_origin = this->graph->edge_num * 2, cnt = 0;
	for(Edge *edge = this->edges; edge != this->end_edge && cnt < edge_num_origin; edge++)
	{
		int e_id = edge->id;
		//cout<<e_id<<" "<<e_cap[e_id]<<endl;
		if(edge->ident == BASIC_EDGE || edge->ident == EDGE_UPPER) e_flow[e_id] = edge->flow;
		else e_flow[e_id] = 0;
		cnt++;
	}
	//cout<<"edge cnt: "<<cnt<<endl;
}

inline int* NetSimplexMCF::get_edge_flow() { return e_flow; }

inline vector<pair<int, int> >* NetSimplexMCF::get_server_flow() { return &(this->server_flow); }

void NetSimplexMCF::create_init_base()
{
	for(Edge *edge = this->edges; edge != this->end_edge; edge++)
	{
		edge->flow = 0;
		edge->ident = EDGE_LOWER;
	}

	for(Edge *edge = this->virtual_edges; edge != this->end_virtual_edge; edge++)
	{
		Node *node = this->nodes + (edge - this->virtual_edges);
		if(node->balance > 0)
		{
			edge->last = this->virtual_root;
			edge->first = node;
			edge->flow = node->balance;
		}
		else
		{
			edge->last = node;
			edge->first = this->virtual_root;
			edge->flow = -node->balance;
		}

		edge->cost = this->virtual_edge_max_cost;
		edge->ident = BASIC_EDGE;
		edge->cap = INF;
	}

	this->virtual_root->balance = 0;
	this->virtual_root->potential = this->virtual_edge_max_cost;
	this->virtual_root->sub_tree_lv = 0;
	this->virtual_root->entering_edge = NULL;
	this->virtual_root->prev = NULL;
	this->virtual_root->next = this->nodes;
	for(Node *node = this->nodes; node != this->end_node; node++)
	{
		node->potential = node->balance > 0 ? this->virtual_edge_max_cost * 2 : 0;
		node->sub_tree_lv = 1;
		node->entering_edge = this->virtual_edges + (node - this->nodes);
		node->prev = node - 1;
		node->next = node + 1;
	}

	(this->nodes + n - 1)->next = NULL;
	this->nodes->prev = this->virtual_root;
}

void NetSimplexMCF::run_simplex()
{
	Edge *entering_edge, *leaving_edge;

	this->n_candidate_lists = ((this->m - 1) / this->n_candidate_list) + 1;
	this->real_candidate_list = this->tmp_candidate_list_sz = 0;

	while(this->status == UNSOLVED)
	{
		entering_edge = rule_candidate_list_pivot();

		if(entering_edge)
		{
			Edge *edge;
			Node *k1, *k2;

			int t, alpha;
			if(entering_edge->ident == EDGE_UPPER)
			{
				k1 = entering_edge->first;
				k2 = entering_edge->last;
				alpha = entering_edge->flow;
			}
			else
			{
				k1 = entering_edge->last;
				k2 = entering_edge->first;
				alpha = entering_edge->cap - entering_edge->flow;
			}

			Node *mem_k1 = k1, *mem_k2 = k2;
			leaving_edge = NULL;
			bool leaving_reduces_flow = reduce_cost(entering_edge) > 0 ;
			bool leave;
			while(k1 != k2)
			{
				if(k1->sub_tree_lv > k2->sub_tree_lv)
				{
					edge = k1->entering_edge;
					if(edge->last != k1) t = edge->cap - edge->flow, leave = false;
					else t = edge->flow, leave = true;

					if(t < alpha)
					{
						alpha = t, leaving_edge = edge;
						leaving_reduces_flow = leave;
					}

					k1 = get_father(k1 , edge);
				}
				else
				{
					edge = k2->entering_edge;
					if(edge->last == k2) t = edge->cap - edge->flow, leave = false;
					else t = edge->flow, leave = true;

					if(t <= alpha)
					{
						alpha = t, leaving_edge = edge;
						leaving_reduces_flow = leave;
					}

					k2 = get_father(k2, edge);
				}
			}
			if(!leaving_edge) leaving_edge = entering_edge;
			if(alpha >= INF) { this->status = UNBOUNDED; break; }

			k1 = mem_k1; k2 = mem_k2;
			if(alpha)
			{
				entering_edge->flow += entering_edge->last == k1 ? alpha : -alpha;
				while(k1 != k2)
				{
					if(k1->sub_tree_lv > k2->sub_tree_lv)
					{
						edge = k1->entering_edge;
						edge->flow += edge->last != k1 ? alpha : -alpha;
						k1 = get_father(k1, k1->entering_edge);
					}
					else
					{
						edge = k2->entering_edge;
						edge->flow += edge->last == k2 ? alpha : -alpha;
						k2 = get_father(k2, k2->entering_edge);
					}
				}
			}

			if(entering_edge != leaving_edge)
			{
				bool leaving_bring_flow_in_T2 = true;
				if(leaving_edge->last->sub_tree_lv > leaving_edge->first->sub_tree_lv && !leaving_reduces_flow)
				{
					leaving_bring_flow_in_T2 = false;
				}
				if(leaving_edge->last->sub_tree_lv <= leaving_edge->first->sub_tree_lv && leaving_reduces_flow)
				{
					leaving_bring_flow_in_T2 = false;
				}
				if(leaving_bring_flow_in_T2 == (mem_k1 == entering_edge->last))
				{
					k2 = entering_edge->last, k1 = entering_edge->first;
				}
				else
				{
					k2 = entering_edge->first, k1 = entering_edge->last;
				}
			}

			leaving_edge->ident = leaving_reduces_flow ? EDGE_LOWER : EDGE_UPPER;

			if (leaving_edge != entering_edge)
			{
				entering_edge->ident = BASIC_EDGE;
				Node *h1, *h2;
				if(leaving_edge->last->sub_tree_lv < leaving_edge->first->sub_tree_lv)
				{
					h1 = leaving_edge->last, h2 = leaving_edge->first;
				}
				else
				{
					h1 = leaving_edge->first, h2 = leaving_edge->last;
				}

				update_T(h1, h2, k1, k2, leaving_edge, entering_edge);
				k2 = entering_edge->first;
				int dt = reduce_cost(entering_edge);
				if(entering_edge->last->sub_tree_lv > entering_edge->first->sub_tree_lv)
				{
					dt = -dt, k2 = entering_edge->last;
				}

				update_potential(dt, k2);
			}
		}
		else
		{
			this->status = OPTIMAL_SOLVED;
			for(Edge *edge = this->virtual_edges; edge != this->end_virtual_edge; edge++ )
				if(edge->flow > 0) this->status = UNFEASIBLE;
		}
	}
}

Edge* NetSimplexMCF::rule_candidate_list_pivot()
{
	int min_val = this->hot_list_sz < this->tmp_candidate_list_sz ? this->hot_list_sz : this->tmp_candidate_list_sz;

	int next = 0, i;
	for(i = 2; i <= min_val; ++i)
	{
		Edge *edge = this->candidates[i].edge;
		int red_cost = reduce_cost(edge);

		if((red_cost < 0 && edge->ident == EDGE_LOWER) || (red_cost > 0 && edge->ident == EDGE_UPPER))
		{
			next++;
			this->candidates[ next ].edge = edge;
			this->candidates[ next ].abs_rc = red_cost >=0 ? red_cost : -red_cost;
		}
	}

	this->tmp_candidate_list_sz = next;
	int old_candidate_list = this->real_candidate_list;
	do{
		for(Edge *edge = this->edges + this->real_candidate_list; edge < this->end_edge; edge += this->n_candidate_lists)
		{
			if(edge->ident == EDGE_LOWER)
			{
				int red_cost = reduce_cost(edge);
				if(red_cost < 0)
				{
					this->tmp_candidate_list_sz++;
					this->candidates[this->tmp_candidate_list_sz].edge = edge;
					this->candidates[this->tmp_candidate_list_sz].abs_rc = red_cost >= 0 ? red_cost : -red_cost;
				}
			}
			else if(edge->ident == EDGE_UPPER)
			{
				int red_cost = reduce_cost(edge);
				if(red_cost > 0)
				{
					this->tmp_candidate_list_sz++;
					this->candidates[this->tmp_candidate_list_sz].edge = edge;
					this->candidates[this->tmp_candidate_list_sz].abs_rc = red_cost >= 0 ? red_cost : -red_cost;
				}
			}
		}

		this->real_candidate_list++;
		if(this->real_candidate_list == this->n_candidate_lists) this->real_candidate_list = 0;
	}while(this->tmp_candidate_list_sz < this->hot_list_sz && this->real_candidate_list != old_candidate_list);

	if(this->tmp_candidate_list_sz)
	{
		quick_sort_candidate_list(1 , this->tmp_candidate_list_sz);
		return this->candidates[ 1 ].edge;
	}
	else return NULL;
}

void NetSimplexMCF::update_T(Node *h1, Node *h2, Node *k1, Node *k2, Edge *h, Edge *k)
{
	int dt = (k1->sub_tree_lv) + 1 - (k2->sub_tree_lv);

	Node *root = k2, *father, *prev_node = k1, *last_node;
	Edge *edge1 = k, *edge2;

	bool flag = false;
	while(!flag)
	{
		if(root == h2) flag = true;

		father = get_father(root ,root->entering_edge);
		last_node = cut_and_update_sub_tree(dt, root);
		paste_sub_tree(last_node, prev_node, root);
		prev_node = last_node;
		dt += 2;

		edge2 = root->entering_edge;
		root->entering_edge = edge1;
		edge1 = edge2;
		root = father;
	}
}

inline void NetSimplexMCF::update_potential(int dt, Node *root)
{
	int level = root->sub_tree_lv;
	Node *node = root;

	do{
		node->potential += dt;
		node = node->next;
	}while(node && node->sub_tree_lv > level);
}

inline void NetSimplexMCF::quick_sort_candidate_list(int min ,int max)
{
	int left = min, right = max;

	int cut = this->candidates[(left + right) >> 1].abs_rc;
	do{
		while(this->candidates[left].abs_rc > cut) left++;
		while(cut > this->candidates[right].abs_rc) right--;

		if(left < right) swap_candidate(this->candidates[left] , this->candidates[right]);

		if(left <= right)
		{
			left++, right--;
		}
	}while(left <= right);

	if(min < right) quick_sort_candidate_list(min ,right);
	if(left < max && left <= this->hot_list_sz) quick_sort_candidate_list(left ,max);
}

inline void NetSimplexMCF::swap_candidate(Candidate_T &candidate1 ,Candidate_T &candidate2)
{
	Candidate_T tmp = candidate1;
	candidate1 = candidate2;
	candidate2 = tmp;
}

inline int NetSimplexMCF::reduce_cost(Edge *edge)
{
	int red_cost = (edge->last)->potential - (edge->first)->potential;
	return red_cost + edge->cost;
}

inline Node* NetSimplexMCF::get_father(Node *node ,Edge *edge)
{
	if(!edge) return NULL;
	return edge->last == node ? edge->first : edge->last;
}

Node* NetSimplexMCF::cut_and_update_sub_tree(int dt, Node *root)
{
	int level = root->sub_tree_lv;
	Node *node = root;
	while(node->next && node->next->sub_tree_lv > level)
	{
		node = node->next;
		node->sub_tree_lv += dt;
	}
	root->sub_tree_lv += dt;

	if(root->prev) root->prev->next = node->next;
	if(node->next) node->next->prev = root->prev;

	return node;
}

void NetSimplexMCF::paste_sub_tree(Node *last_node, Node *prev_node, Node *root)
{
	Node *next_node = prev_node->next;
	root->prev = prev_node;
	prev_node->next = root;
	last_node->next = next_node;
	if(next_node) next_node->prev = last_node;
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
            //total_cost += g_node_cost[server_id] + g_server_level[level].cost;
        }
        /*for(int i = 0; i < server_flow.size(); i++)
        {
            server_id = server_flow[i].first;
            flow = server_flow[i].second;
            level = g_server_level_index[flow];
            //cout<<"server: "<<flow<<" "<<server_id<<" "<<level<<endl;
            if(flow == 0) continue;
            total_cost += g_node_cost[server_id] + g_server_level[level].cost;
                //total_cost += g_node_cost[server_id] + g_server_level[servers[server_id]].cost;
        }*/
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
