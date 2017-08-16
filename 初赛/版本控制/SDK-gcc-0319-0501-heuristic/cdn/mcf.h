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

using namespace std;

struct MyEdge
{
	int to, next, cap, flow, cost;
}edge[MAX_EDGE_NUMBER];

class MCF
{
public:
	Graph *graph;

	int head[MAX_NODE_NUMBER], tol;
	int pre[MAX_NODE_NUMBER], dis[MAX_NODE_NUMBER];
	bool vis[MAX_NODE_NUMBER];
	int N;

	int node_out_flow[MAX_NODE_NUMBER];
	int node_cost[MAX_NODE_NUMBER];

	bool mark[MAX_NODE_NUMBER];

	map<int, int> mp;

	map<int, int> mp_net_to_cus;

	MCF(Graph *graph);

	bool compute(vector<Cus_Node *> &cus_nodes);
	void construct_solution(vector<Cus_Node *> &final_cus_nodes, vector<stack<int> > &paths);

private:
	void init(vector<Cus_Node *> &cus_nodes);
	void add_edge(int u, int v, int cap, int cost);
	bool spfa(int s, int t);
	int min_cost_max_flow(int s, int t, int &cost);
	void dfs_record(int u);
	void dfs_construct_path(int u, vector<int> path_edges, vector<stack<int> > &paths);
};


MCF::MCF(Graph *graph)
{
	this->graph = graph;
	this->N = graph->net_node_num+1;

	for(Cus_Node *node : graph->cus_nodes)
	{
		mp_net_to_cus[node->des] = node->src;
	}
}

void MCF::init(vector<Cus_Node *> &cus_nodes)
{
	tol = 0;
	memset(head, -1, sizeof(head));

	memset(node_out_flow, 0, sizeof(node_out_flow));
	memset(node_cost, 0, sizeof(node_cost));

	for(Edge *e : graph->net_edges)
	{
		this->add_edge(e->src, e->des, e->cap, e->fee);
	}

	for(Cus_Node *node : cus_nodes)
	{
		this->add_edge(node->des, N-1, node->need, 0);
	}
}

void MCF::add_edge(int u, int v, int cap, int cost)
{
	if(mp.find(tol) != mp.end())
	{
		cap -= mp[tol];
	}

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

int MCF::min_cost_max_flow(int s, int t, int &cost)
{
	int flow = 0;
	cost = 0;
	while(spfa(s, t))
	{
		int Min = INF;
		for(int i = pre[t]; i!=-1; i=pre[edge[i^1].to])
		{
			if(Min > edge[i].cap-edge[i].flow)
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

			node_out_flow[edge[i].to] += Min;
			tmp += edge[i].cost*Min;
			node_cost[edge[i^1].to] += tmp;
		}
		node_out_flow[s] += Min;

		flow += Min;
	}
	return flow;
}

bool comp(Cus_Node *node1, Cus_Node *node2)
{
	return node1->need < node2->need;
}

bool MCF::compute(vector<Cus_Node *> &cus_nodes)
{
	vector<Cus_Node *> cus_r;
	for(int i=0; i<cus_nodes.size(); ++i)
	{
		cus_r.push_back(cus_nodes[i]);
	}

	sort(cus_r.begin(), cus_r.end(), comp);

	int cnt = 0;

	while(cus_r.size()>=2)
	{
		memset(mark, 0, sizeof(mark));

		for(int s=0; s<N-1; ++s)
		{

			if(mark[s]) continue;

			int cost, flow, t = N-1;
			init(cus_r);

			flow =  min_cost_max_flow(s, t, cost);
			mark[s] = true;

			// check
			bool flag = true;
			for(int i=head[t]; i!=-1; i=edge[i].next)
			{
				if(edge[i^1].flow < edge[i^1].cap)
				{
					flag = false;
					break;
				}
			}
			if(!flag) continue;

			int u = cus_r[0]->des;
			while(node_out_flow[u]<flow)
			{
				for(int i=head[u]; i!=-1; i=edge[i].next)
				{
					if(edge[i^1].flow > 0)
					{
						u = edge[i].to;
						break;
					}
				}
			}
			if(node_cost[u]+this->graph->net_fee < cus_r.size()*this->graph->net_fee)
			{
				// update cus_nodes
				set<Cus_Node *> st;
				for(int i=0; i<cus_r.size(); ++i)
				{
					st.insert(cus_r[i]);
				}
				vector<Cus_Node *> tmp;
				for(int i=0; i<cus_nodes.size(); ++i)
				{
					Cus_Node *node = cus_nodes[i];
					if(st.find(node)==st.end()&&node->des!=u)
					{
						tmp.push_back(new Cus_Node(node->src, node->des, node->need));
					}
				}
				cus_nodes.clear();
				for(int i=0; i<tmp.size(); ++i)
				{
					cus_nodes.push_back(new Cus_Node(tmp[i]->src, tmp[i]->des, tmp[i]->need));
				}
				cus_nodes.push_back(new Cus_Node(-1, u, flow));

				// record edge flow and modify edge capacity
				dfs_record(u);

				return true;
			}
			else
			{
				vis[u] = true;
			}

		}

		cus_r.pop_back();

	}

	return false;
}

void MCF::dfs_record(int u)
{
	for(int i=head[u]; i!=-1; i=edge[i].next)
	{
		if(edge[i].flow>0 && edge[i].to!=N-1)
		{
			if(mp.find(i) == mp.end())
			{
				mp[i] = edge[i].flow;
			}
			else
			{
				mp[i] += edge[i].flow;
			}
			
			dfs_record(edge[i].to);
		}
	}
}


void MCF::construct_solution(vector<Cus_Node *> &final_cus_nodes, vector<stack<int> > &paths)
{
	mp.clear();
	N++;
	init(graph->cus_nodes);
	for(Cus_Node *node : final_cus_nodes)
	{
		this->add_edge(N-2, node->des, INF, 0);
	}
	int flow, cost;
	flow = min_cost_max_flow(N-2, N-1, cost); // N-2 is the super source, N-1 is the super sink
	cost += final_cus_nodes.size()*graph->net_fee;

	for(Cus_Node *node : final_cus_nodes)
	{
		int u = node->des;
		bool flag = false;
		for(int i=head[u]; i!=-1; i=edge[i].next)
		{
			if(edge[i].flow > 0)
			{
				flag = true;
				break;
			}
		}
		if(!flag)
		{
			cost -= graph->net_fee;
			continue;
		}

		vector<int> path_edges;
		dfs_construct_path(u, path_edges, paths);
	}

	//cout<<"\nResult:"<<endl;
	//cout<<"flow = "<<flow<<", cost = "<<cost<<endl;
}

void MCF::dfs_construct_path(int u, vector<int> path_edges, vector<stack<int> > &paths)
{
	if(u==N-1)
	{
		stack<int> stk;
		int b = INF;
		for(int i=0; i<path_edges.size(); ++i)
		{
			b = min(b, edge[path_edges[i]].flow);
		}
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