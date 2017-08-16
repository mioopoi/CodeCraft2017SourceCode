#ifndef _MIN_COST_FLOW_H_
#define _MIN_COST_FLOW_H_

#include <iostream>
#include <cstdio>
#include <cstring>
#include <queue>

#include "graph.h"
#include "globals.h"

using namespace std;

struct FlowEdge
{
    int to, next, cap, cost, flow;
};

class MinCostFlow
{
public:
    int source;    // 超源点
    int sink;      // 超汇点
    int tol;       // 计数变量
    int n;         // 节点总数量（包括超源点和超汇点）
    int m;         // 有向边总数量（不包括虚拟边）
    int min_cost;  // 最小费用
    int max_flow;  // 最大流
    int sum_demand;
    Graph* graph;  // 原图
    FlowEdge edge[MAX_EDGE_NUMBER];
    int head[MAX_NODE_NUMBER];
    int pre[MAX_NODE_NUMBER];
    int dis[MAX_NODE_NUMBER];
    bool vis[MAX_NODE_NUMBER];
    MinCostFlow(Graph *graph);
    void init(vector<bool>& chromosome);  // 根据个体染色体初始化新图（首先将残余网络恢复到原始状态）
    bool compute();  // 计算最小费用流主函数
    void print();
private:
    void add_edge(int u, int v, int cap, int cost);  // 添加有向边，同时构造残余网络
    bool spfa();
};

MinCostFlow::MinCostFlow(Graph* graph)
{
    cout << "Hello MinCostFlow!"<< endl;
    this->graph = graph;
    this->source = graph->net_node_num;   // 添加超源点
    this->sink = graph->net_node_num + 1; // 添加超汇点
    this->tol = 0;
    this->n = graph->net_node_num + 2;
    this->m = graph->edge_num * 2;
    this->min_cost = INF;
    this->max_flow = 0;
    this->sum_demand = 0;
    memset(head, -1, sizeof(head));
    for (Edge* e : graph->net_edges)
    {
        this->add_edge(e->src, e->des, e->cap, e->fee);
    }
    // 添加有向边：所有demand节点->超汇点
    for (Cus_Node* node : graph->cus_nodes)
    {
        this->add_edge(node->des, this->sink, node->need, 0);
        this->sum_demand += node->need;
    }
    cout << "total edges: " << tol << endl;
}

void MinCostFlow::add_edge(int u, int v, int cap, int cost)
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

void MinCostFlow::init(vector<bool>& chromosome)
{
    // 参数重置
    this->min_cost = INF;
    this->max_flow = 0;
    for (int i = 0; i < this->tol; ++i)
    {
        edge[i].flow = 0;
    }
    this->tol = (this->m + this->graph->cus_node_num) * 2;
    for (int i = head[source]; i != -1; i = edge[i].next)
    {
        int v = edge[i].to;
        if (head[v] != -1)
        {
            head[v] = edge[head[v]].next;  // 删除超源点指向节点v的虚拟边
        }
    }
    head[source] = -1;

    // 添加有向边：超源点->所有supply节点
    for (int i = 0; i < this->graph->net_node_num; ++i)
    {
        if (chromosome[i])  // 节点i是supply节点
        {
            this->add_edge(this->source, i, INF, 0);
        }
    }
    //cout << tol << endl;
}

bool MinCostFlow::spfa()
{
    queue<int> q;
    for (int i = 0; i < this->n; ++i)
    {
        dis[i] = INF;
        vis[i] = false;
        pre[i] = -1;
    }
    dis[source] = 0;
    vis[source] = true;
    q.push(source);
    while (!q.empty())
    {
        int u = q.front();
        q.pop();
        vis[u] = false;
        for (int i = head[u]; i != -1; i = edge[i].next)
        {
            int v = edge[i].to;
            if (edge[i].cap > edge[i].flow && dis[v] > dis[u] + edge[i].cost)
            {
                dis[v] = dis[u] + edge[i].cost;
                pre[v] = i;
                if (!vis[v])
                {
                    vis[v] = true;
                    q.push(v);
                }
            }
        }
    }
    return !(pre[sink] == -1);
}

bool MinCostFlow::compute()
{
    int flow = 0, cost = 0;
    while (spfa())
    {
        int aug_flow = INF;
        for (int i = pre[sink]; i != -1; i = pre[edge[i^1].to])
        {
            int remain_cap = edge[i].cap - edge[i].flow;
            if (aug_flow > remain_cap)
            {
                aug_flow = remain_cap;
            }
        }
        for (int i = pre[sink]; i != -1; i = pre[edge[i^1].to])
        {
            edge[i].flow += aug_flow;
            edge[i^1].flow -= aug_flow;
            cost += edge[i].cost * aug_flow;
        }
        flow += aug_flow;
    }
    this->min_cost = cost;
    this->max_flow = flow;
    return flow == this->sum_demand;
}

void MinCostFlow::print()
{
	cout << min_cost<< " "<<max_flow<<endl;
	for (int i = 0; i < tol; ++i)
	{
	    cout << "from: " << edge[i^1].to << " to: "<< edge[i].to << " flow: " << edge[i].flow << endl;
	}
}









#endif  // _MIN_COST_FLOW_H
