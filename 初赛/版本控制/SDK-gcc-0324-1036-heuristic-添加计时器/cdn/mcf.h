#ifndef __MCF_H__
#define __MCF_H__

#include <cstdio>
#include <cstring>
#include <vector>
#include <stack>
#include <queue>
#include <set>
#include <map>
#include <algorithm>

#include "globals.h"
#include "graph.h"

#include <ctime>

using namespace std;

struct MyEdge
{
	int to, next, cap, flow, cost;
}edge[MAX_EDGE_NUMBER], edge_final[MAX_EDGE_NUMBER];

class MCF
{
public:
	Graph *graph;

	int head[MAX_NODE_NUMBER], tol;
	int pre[MAX_NODE_NUMBER], dis[MAX_NODE_NUMBER];
	bool vis[MAX_NODE_NUMBER];
	int N;

	int source;
	int sink;

	int cus_cost[MAX_NODE_NUMBER];

	int deploy_cost[MAX_NODE_NUMBER];

	map<int, int> mp_net_to_cus;

	int max_flow;
	int min_cost;

	set<int> servers;

	clock_t time_start;
	clock_t time_now;

	MCF(Graph *graph);

	bool compute(vector<Cus_Node *> &cus_nodes);
	void construct_solution(vector<Cus_Node *> &final_cus_nodes, vector<stack<int> > &paths);

private:
	void add_edge(int u, int v, int cap, int cost);
	void add_source(vector<Cus_Node *> &cus_nodes);
	void add_sink(vector<Cus_Node *> &cus_nodes);

	void remove_source();
	void remove_sink();

	bool spfa(int s, int t);
	int min_cost_max_flow(int s, int t, int &cost, bool update_deploy_cost);

	int get_replace_gain(int u, set<int> &current_cus_set, set<int> &old_nodes);

	void dfs_construct_path(int u, vector<int> path_edges, vector<stack<int> > &paths);
};


MCF::MCF(Graph *graph)
{
	this->graph = graph;
	this->N = graph->net_node_num+2;

	this->source = N-2;
	this->sink = N-1;

	for(Cus_Node *node : graph->cus_nodes)
	{
		mp_net_to_cus[node->des] = node->src;

		deploy_cost[node->des] = graph->net_fee;
	}

	this->tol = 0;

	memset(head, -1, sizeof(head));

	for(Edge *e : graph->net_edges)
	{
		this->add_edge(e->src, e->des, e->cap, e->fee);
	}

	for(int i=0; i<this->tol; ++i)
	{
		edge_final[i].to = edge[i].to;
		edge_final[i].next = edge[i].next;
		edge_final[i].cap = edge[i].cap;
		edge_final[i].flow = edge[i].flow;
		edge_final[i].cost = edge[i].cost;
	}

	this->max_flow = 0;
	this->min_cost = 0;

	this->time_start = clock();
}

void MCF::add_edge(int u, int v, int cap, int cost)
{
	edge[tol].to = v;
	edge[tol].cap = cap;
	edge[tol].cost = cost;
	edge[tol].flow = 0;
	edge[tol].next = head[u];
	head[u] = tol++;

	edge[tol].to = u;
	edge[tol].cap = 0;
	edge[tol].cost = -cost;
	edge[tol].flow = 0;
	edge[tol].next = head[v];
	head[v] = tol++;
}

void MCF::add_source(vector<Cus_Node *> &servers)
{
	for(Cus_Node *node : servers)
	{
		this->add_edge(this->source, node->des, INF, 0);
	}
}

void MCF::add_sink(vector<Cus_Node *> &cus_nodes)
{
	for(Cus_Node *node : cus_nodes)
	{
		this->add_edge(node->des, this->sink, node->need, 0);
	}
}

void MCF::remove_source()
{
	for(int i=head[source]; i!=-1; i=edge[i].next)
	{
		int v = edge[i].to;
		if(head[v] != -1)
		{
			head[v] = edge[head[v]].next;
			tol -= 2;
		}
	}
	head[source] = -1;
}

void MCF::remove_sink()
{
	for(int i=head[sink]; i!=-1; i=edge[i].next)
	{
		int v = edge[i].to;
		if(head[v] != -1)
		{
			head[v] = edge[head[v]].next;
			tol -= 2;
		}
	}
	head[sink] = -1;
}

bool MCF::spfa(int s, int t)
{
	queue<int> q;
	for(int i=0; i<N; ++i)
	{
		dis[i] = INF;
		vis[i] = false;
		pre[i] = -1;
	}
	dis[s] = 0;
	vis[s] = true;
	q.push(s);
	while(!q.empty())
	{
		int u = q.front();
		q.pop();
		vis[u] = false;
		for(int i=head[u]; i!=-1; i=edge[i].next)
		{
			int v = edge[i].to;
			if(edge[i].cap > edge[i].flow && dis[v] > dis[u]+edge[i].cost)
			{
				dis[v] = dis[u]+edge[i].cost;
				pre[v] = i;
				if(!vis[v])
				{
					vis[v] = true;
					q.push(v);
				}
			}
		}
	}
	if(pre[t] == -1) return false;
	else return true;
}

int MCF::min_cost_max_flow(int s, int t, int &cost, bool update_deploy_cost=false)
{
	int flow = 0;
	cost = 0;
	while(spfa(s, t))
	{
		int Min = INF;
		for(int i = pre[t]; i!=-1; i=pre[edge[i^1].to])
		{
			if(edge[i].cap-edge[i].flow < Min)
			{
				Min = edge[i].cap-edge[i].flow;
			}
		}
		int tmp = 0;
		for(int i=pre[t]; i!=-1; i = pre[edge[i^1].to])
		{
			edge[i].flow += Min;
			edge[i^1].flow -= Min;

			cost += edge[i].cost*Min;

			tmp += edge[i].cost*Min;

			if(update_deploy_cost==true && this->servers.find(edge[i^1].to)!=this->servers.end())
			{
				deploy_cost[edge[i].to] += tmp;
			}
		}

		cus_cost[edge[pre[t]^1].to] += tmp;

		flow += Min;
	}
	return flow;
}

int MCF::get_replace_gain(int u, set<int> &current_cus_set, set<int> &old_nodes)
{
	int gain = 0;
	for(int i=head[this->sink]; i!=-1; i=edge[i].next)
	{
		if(edge[i^1].flow==edge[i^1].cap && cus_cost[edge[i].to] < this->deploy_cost[edge[i].to] && edge[i].to!=u)
		{
			gain += this->deploy_cost[edge[i].to] - cus_cost[edge[i].to];
			old_nodes.insert(edge[i].to);
		}
	}

	gain -= current_cus_set.find(u)==current_cus_set.end() ? graph->net_fee : 0;

	return gain;
}

struct ReplaceSolution
{
	int new_node;
	set<int> old_nodes;
	int replace_gain;

	ReplaceSolution(int new_node, set<int> &old_nodes, int replace_gain)
	{
		this->new_node = new_node;
		this->replace_gain = replace_gain;
		for(set<int>::iterator it=old_nodes.begin(); it!=old_nodes.end(); it++)
		{
			this->old_nodes.insert(*it);
		}
	}

	bool operator<(const ReplaceSolution& sol)const
	{
		return this->replace_gain > sol.replace_gain;
	}
};

bool MCF::compute(vector<Cus_Node *> &cus_nodes)
{
	set<int> current_cus_set;
	for(int i=0; i<cus_nodes.size(); ++i)
	{
		current_cus_set.insert(cus_nodes[i]->des);
	}

	vector<ReplaceSolution> replace_sols;
	int replace_gain = 0;
	set<int> old_nodes;

	for(int j=0; j<graph->net_node_num; ++j)
	//for(int k=0; k<cus_nodes.size(); ++k)
	{
		//int j = cus_nodes[k]->des;

		for(int i=0; i<tol; ++i)
		{
			edge[i].flow = edge_final[i].flow;
		}
		add_sink(cus_nodes);

		memset(cus_cost, 0, sizeof(cus_cost));

		int s = j, t = this->sink;
		int flow, cost;

		flow = min_cost_max_flow(s, t, cost);

		old_nodes.clear();
		replace_gain = get_replace_gain(s, current_cus_set, old_nodes);
		if(replace_gain > 0 && old_nodes.size() > 0)
		{
			replace_sols.push_back(ReplaceSolution(s, old_nodes, replace_gain));
		}
		remove_sink();

		this->time_now = clock();
		if((double(this->time_now-this->time_start)/CLOCKS_PER_SEC) > 85) break;
	}

	sort(replace_sols.begin(), replace_sols.end());

	if(replace_sols.size()>0)
	{
		int new_node = replace_sols[0].new_node;
		set<int> &old_nodes = replace_sols[0].old_nodes;

		for(int i=0; i<tol; ++i)
		{
			edge[i].flow = 0;
		}

		add_sink(graph->cus_nodes);

		vector<Cus_Node *> servers;
		this->servers.clear();
		for(Cus_Node *node : cus_nodes)
		{
			if(old_nodes.find(node->des)==old_nodes.end() && node->des!=new_node)
			{
				servers.push_back(new Cus_Node(-1, node->des, 0));
				this->servers.insert(node->des);
				this->deploy_cost[node->des] = graph->net_fee;
			}
		}
		servers.push_back(new Cus_Node(-1, new_node, 0));
		this->servers.insert(new_node);
		this->deploy_cost[new_node] = graph->net_fee;
		add_source(servers);

		int s = this->source, t = this->sink;
		int flow, cost;

		flow = min_cost_max_flow(s, t, cost, true);

		/*cout<<"replace_gain = "<<replace_sols[0].replace_gain<<endl;
		cout<<"flow = "<<flow<<", cost = "<<cost+servers.size()*graph->net_fee<<endl;
		cout<<"new_node = "<<new_node<<endl;
		cout<<"old_nodes:"<<endl;
		for(set<int>::iterator it=old_nodes.begin(); it!=old_nodes.end(); it++)
		{
			cout<<*it<<" ";
		}
		cout<<endl;
		getchar();*/

		cus_nodes.clear();
		for(int i=head[this->source]; i!=-1; i=edge[i].next)
		{
			if(edge[i].flow > 0)
			{
				cus_nodes.push_back(new Cus_Node(-1, edge[i].to, edge[i].flow));
			}
		}

		/*cout<<"cus_nodes: ";
		for(int i=0; i<cus_nodes.size(); ++i)
		{
			cout<<cus_nodes[i]->des<<" ";
		}
		cout<<endl;
		getchar();*/

		remove_source();
		remove_sink();
		for(int i=0; i<tol; ++i)
		{
			edge_final[i].flow = edge[i].flow;
		}

		this->time_now = clock();
		if((double(this->time_now-this->time_start)/CLOCKS_PER_SEC) > 85) return false;

		return true;
	}

	return false;
}

void MCF::construct_solution(vector<Cus_Node *> &final_cus_nodes, vector<stack<int> > &paths)
{
	int flow, cost;

	for(int i=0; i<tol; ++i)
	{
		edge[i].flow = edge_final[i].flow;
	}
	add_sink(graph->cus_nodes);

	for(int i=head[sink]; i!=-1; i=edge[i].next)
	{
		edge[i^1].flow = edge[i^1].cap;
		edge[i].flow = -edge[i^1].cap;
	}

	for(Cus_Node *node : final_cus_nodes)
	{
		int u = node->des;
		vector<int> path_edges;

		dfs_construct_path(u, path_edges, paths);
	}

	this->min_cost += final_cus_nodes.size()*this->graph->net_fee;

	cout<<"\nmax_flow = "<<this->max_flow<<endl;
	cout<<"min_cost = "<<this->min_cost<<endl;
	cout<<endl;
}

void MCF::dfs_construct_path(int u, vector<int> path_edges, vector<stack<int> > &paths)
{
	if(u==this->sink)
	{
		stack<int> stk;
		int b = INF;
		for(int i=0; i<path_edges.size(); ++i)
		{
			b = min(b, edge[path_edges[i]].flow);
		}

		for(int i=0; i<path_edges.size(); ++i)
		{
			this->min_cost += b*edge[path_edges[i]].cost;
		}
		this->max_flow += b;

		stk.push(b);
		int i=path_edges.size()-1;
		stk.push(mp_net_to_cus[edge[path_edges[i]^1].to]);
		for(; i>=0; --i)
		{
			stk.push(edge[path_edges[i]^1].to);
			edge[path_edges[i]].flow -= b;
		}
		paths.push_back(stk);
	}

	for(int i=head[u]; i!=-1; i=edge[i].next)
	{
		if(edge[i].flow > 0)
		{
			int _u = edge[i].to;
			vector<int> _path_edges;
			for(int j=0; j<path_edges.size(); ++j)
			{
				_path_edges.push_back(path_edges[j]);
			}
			_path_edges.push_back(i);

			dfs_construct_path(_u, _path_edges, paths);
		}
	}
}

#endif