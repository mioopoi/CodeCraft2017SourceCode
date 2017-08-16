#ifndef _MIN_COST_FLOW_C_H_
#define _MIN_COST_FLOW_C_H_

#include <iostream>
#include <cstdio>
#include <cstring>
#include <queue>
#include <stack>

#include "graph.h"
#include "globals.h"
#include "mcf_helper.h"

using namespace std;

class MinCostFlowC
{
public:
    int source;    // 超源点
    int sink;      // 超汇点
    int tol;       // 计数变量
    int node_num;         // 节点总数量（包括超源点和超汇点）
    int edge_num;         // 有向边总数量（不包括虚拟边）
    int max_edge_num;

    int min_cost;  // 最小费用
    int max_flow;  // 最大流
    int sum_demand;

    int head[MAX_NODE_NUMBER];
    int pre[MAX_NODE_NUMBER];
    int dis[MAX_NODE_NUMBER];
    bool vis[MAX_NODE_NUMBER];

    Graph* graph;  // 原图

    FlowEdgeC* edges_storage;          //内存提前分配
    FlowEdgeC* edges_storage_copy;     //指针保存
    FlowEdgeC** all_edges;

    MinCostFlowC(Graph *graph);
    void add_edge(int src, int des, int cap, int cost);
    void init(vector<bool>& servers);  // 根据个体染色体初始化新图（首先将残余网络恢复到原始状态）
    bool run_min_cost_flow();                       // 计算最小费用流主函数
    bool spfa();
    int is_all_satisfied();
    ~MinCostFlowC();

    bool bfs();
    void construct_solution(vector<stack<int> >& paths);

    /* zkw */
    int min_delta_cost;
    int zkw_aug(int src, int flow);
    bool run_zkw_mcf();
    bool zkw_modlabel();
    /* */
};

MinCostFlowC::MinCostFlowC(Graph* graph)
{
    cout << "Hello MinCostFlow!"<< endl;
    this->graph = graph;
    this->source = graph->net_node_num;   // 添加超源点
    this->sink = graph->net_node_num + 1; // 添加超汇点
    this->tol = 0;
    this->node_num = graph->net_node_num + 2;
    this->edge_num = graph->edge_num * 2;
    this->min_cost = INF;
    this->max_flow = 0;
    this->sum_demand = 0;

    memset(head, -1, sizeof(head));

    this->max_edge_num = graph->net_node_num * 2 + graph->cus_node_num * 2 + graph->edge_num * 4;
    this->edges_storage = new FlowEdgeC[max_edge_num];
    this->edges_storage_copy = this->edges_storage;
    this->all_edges = new FlowEdgeC*[max_edge_num];

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

void MinCostFlowC::add_edge(int src, int des, int cap, int cost)
{
    all_edges[tol] = new (this->edges_storage_copy + tol) FlowEdgeC(src, des, head[src], cap, cost);
    head[src] = tol++;

    all_edges[tol] = new (this->edges_storage_copy + tol) FlowEdgeC(des, src, head[des], 0, -cost);
    head[des] = tol++;
}

void MinCostFlowC::init(vector<bool>& servers)
{
        // 参数重置
    this->min_cost = INF;
    this->max_flow = 0;
    for (int i = 0; i < this->tol; ++i)
    {
        all_edges[i]->cap = all_edges[i]->ori_cap;
        all_edges[i]->cost = all_edges[i]->ori_cost;
        all_edges[i]->flow = 0;
    }
    this->tol = (this->edge_num + this->graph->cus_node_num) * 2;

    for (int edge_id = head[source]; edge_id != -1; edge_id = all_edges[edge_id]->next)
    {
        int des = all_edges[edge_id]->des;
        if (head[des] != -1)
        {
            head[des] = all_edges[head[des]]->next;  // 删除超源点指向节点v的虚拟边
        }
    }
    head[source] = -1;


    // 添加有向边：超源点->所有supply节点
    for (int i = 0; i < servers.size(); ++i)
    {
        if (servers[i])  // 节点i是supply节点
        {
            this->add_edge(this->source, i, INF, 0);
        }
    }
}

bool MinCostFlowC::run_min_cost_flow()
{
    int flow = 0, cost = 0;
    while (spfa())
    {
        //cout << "lalala... ";
        int aug_flow = INF;
        for (int i = pre[sink]; i != -1; i = pre[all_edges[i^1]->des])
        {
            int remain_cap = all_edges[i]->cap - all_edges[i]->flow;
            if (aug_flow > remain_cap)
            {
                aug_flow = remain_cap;
            }
        }
        for (int i = pre[sink]; i != -1; i = pre[all_edges[i^1]->des])
        {
            all_edges[i]->flow += aug_flow;
            all_edges[i^1]->flow -= aug_flow;
            cost += all_edges[i]->cost * aug_flow;
        }
        flow += aug_flow;
    }
    this->min_cost = cost;
    this->max_flow = flow;
    return flow == this->sum_demand;
}

int MinCostFlowC::zkw_aug(int src, int flow)
{
    if(src == this->sink)
    {
        this->min_cost += this->min_delta_cost * flow;
        this->max_flow += flow;
        return flow;
    }
    vis[src] = 1;
    int tmp = flow;
    for(int i = head[src]; i != -1; i = all_edges[i]->next)
        if(all_edges[i]->cap && !all_edges[i]->cost && !vis[all_edges[i]->des])
        {
            int delta = zkw_aug(all_edges[i]->des, tmp < all_edges[i]->cap ? tmp : all_edges[i]->cap);
            all_edges[i]->cap -= delta;
            all_edges[i]->flow += delta;
            all_edges[i^1]->cap += delta;
            all_edges[i^1]->flow -= delta;
            tmp -= delta;
            if(!tmp) return flow;
        }
    return flow - tmp;
}

bool MinCostFlowC::zkw_modlabel()
{
    int delta = INF;
    for(int u = 0; u < this->node_num; u++)
    {
        if(!vis[u]) continue;
        for(int i = head[u]; i != -1; i = all_edges[i]->next)
            if(all_edges[i]->cap && !vis[all_edges[i]->des] && all_edges[i]->cost < delta)
                delta = all_edges[i]->cost;
    }

    if(delta == INF) return false;
    for(int u = 0; u < this->node_num; u++)
    {
        if(!vis[u]) continue;
        for(int i = head[u]; i != -1; i = all_edges[i]->next)
            all_edges[i]->cost -= delta, all_edges[i^1]->cost += delta;
    }
    this->min_delta_cost += delta;
    return true;
}

bool MinCostFlowC::run_zkw_mcf()
{
    this->min_cost = 0;
    this->max_flow = 0;
    this->min_delta_cost = 0;
    do
    {
        do
        {
            memset(vis, 0, sizeof(vis));
        } while(zkw_aug(this->source, INF));
    } while(zkw_modlabel());

    return this->max_flow == this->sum_demand;
}

bool MinCostFlowC::spfa()
{
    queue<int> q;
    for (int i = 0; i < this->node_num; ++i)
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
        for (int i = head[u]; i != -1; i = all_edges[i]->next)
        {
            int v = all_edges[i]->des;
            if (all_edges[i]->cap > all_edges[i]->flow && dis[v] > dis[u] + all_edges[i]->cost)
            {
                dis[v] = dis[u] + all_edges[i]->cost;
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

int MinCostFlowC::is_all_satisfied()
{
    int max_demand = 0;
    int cus_id = -1;
    for (int i = head[sink]; i != -1; i = all_edges[i]->next)
    {
        //cout<<this->all_edges[i]->flow << " "<<this->all_edges[i^1]->cap<<endl;
        if(this->all_edges[i]->flow != -this->all_edges[i^1]->ori_cap)
        {
            if(this->all_edges[i^1]->ori_cap > max_demand)
            {
                max_demand = this->all_edges[i^1]->ori_cap;
                cus_id = this->all_edges[i]->des;
            }
        }
    }
    return cus_id;
}

MinCostFlowC::~MinCostFlowC()
{
    delete this->edges_storage_copy;
}

void MinCostFlowC::construct_solution(vector<stack<int> >& paths)
{
    // 在流网络上运行BFS构造路径
    // 注意: 必须先计算好最小费用流
    int flow = 0;
    while (bfs())
    {
        int path_bandwidth = INF;
        stack<int> path;
        for (int i = pre[sink]; i != -1; i = pre[all_edges[i^1]->des])
        {
            if (path_bandwidth > all_edges[i]->flow)
            {
                path_bandwidth = all_edges[i]->flow;
            }
        }
        path.push(path_bandwidth);  // 路径带宽
        int id_net_node = all_edges[pre[sink]^1]->des;
        Edge* out_edge = this->graph->node_out_edges[id_net_node][MAX_OUT_EDGE];
        if (out_edge)
            path.push(out_edge->des);  // 消费节点的独立id
        else
            cout << "error! not a valid demand node" << endl;
        for (int i = pre[sink]; i != -1; i = pre[all_edges[i^1]->des])
        {
            if (all_edges[i^1]->des != source)
            {
                path.push(all_edges[i^1]->des);
            }
            all_edges[i]->flow -= path_bandwidth;
        }
        flow += path_bandwidth;
        paths.push_back(path);
    }
    cout << "BFS flow: " << flow << endl;
}

bool MinCostFlowC::bfs()
{
    queue<int> q;
    for (int i = 0; i < this->node_num; ++i)
    {
        vis[i] = false;
        pre[i] = -1;
    }
    vis[source] = true;
    q.push(source);
    while (!q.empty())
    {
        int u = q.front();
        q.pop();
        if (u == sink) return true;
        for (int i = head[u]; i != -1; i = all_edges[i]->next)
        {
            int v = all_edges[i]->des;
            if (all_edges[i]->flow > 0)
            {
                pre[v] = i;
                if (!vis[v])
                {
                    vis[v] = true;
                    q.push(v);
                }
            }
        }
    }
    return false;
}

#endif  // _MIN_COST_FLOW_H
