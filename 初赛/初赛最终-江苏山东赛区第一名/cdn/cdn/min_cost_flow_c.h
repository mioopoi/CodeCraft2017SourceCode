#ifndef _MIN_COST_FLOW_C_H_
#define _MIN_COST_FLOW_C_H_

#include <iostream>
#include <cstdio>
#include <cstring>
#include <queue>
#include <stack>

#include "graph.h"
#include "globals.h"
#include "greedy_helper.h"

using namespace std;

const int MAX_MCF_EDGE = 80000;

int head[MAX_NODE_NUMBER];
int pre[MAX_NODE_NUMBER];
int dis[MAX_NODE_NUMBER];
bool vis[MAX_NODE_NUMBER];

int edge_des[MAX_MCF_EDGE];
int edge_next[MAX_MCF_EDGE];
int edge_ori_cap[MAX_MCF_EDGE];
int edge_cap[MAX_MCF_EDGE];
int edge_cost[MAX_MCF_EDGE];
int edge_flow[MAX_MCF_EDGE];
int edge_ori_cost[MAX_MCF_EDGE];

int neighbor_cnts[MAX_NODE_NUMBER];

int global_cost_matrix[MAX_NODE_NUMBER][MAX_NODE_NUMBER];
int push_time[MAX_NODE_NUMBER];

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

    Graph* graph;  // 原图

    MinCostFlowC(Graph *graph);

    void add_edge(int src, int des, int cap, int cost);
    void init(vector<bool>& servers);  // 根据个体染色体初始化新图（首先将残余网络恢复到原始状态）
    bool run_min_cost_flow();                       // 计算最小费用流主函数
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

    int first_node;
    int last_node;
    int** cost_matrix;

    void init_cost_matrix();
    int zkw_aug_record_cost(int src, int flow);
    bool run_zkw_mcf_record_cost();
    void set_can_supply(vector<bool>& node_array);
    void construct_neighbor_flows(vector<bool>& servers, vector<vector<NeighborFlow>>& neighbor_flows);

    void init_global_cost_matrix();
    int zkw_aug_record_all_cost(int src, int flow, int cost);
    bool run_zkw_mcf_record_all_cost();

    void zkw_increase_one_server(vector<bool>& servers, int server_id);
    bool find_a_negtive_circle();
    bool find_a_negtive_circle_by_priority_queue();
    bool spfa();

    int cicle_cnt;

};

MinCostFlowC::MinCostFlowC(Graph* graph)
{
    //cout << "Hello MinCostFlow!"<< endl;
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


    this->cost_matrix = new int*[this->node_num];
    for(int i = 0; i < this->node_num; i++)
    {
        this->cost_matrix[i] = new int[this->node_num];
    }

    this->init_cost_matrix();
    cicle_cnt = 0;
    //cout << "total edges: " << tol << endl;
}

void MinCostFlowC::init_global_cost_matrix()
{
    for(int i = 0; i < this->node_num - 2; i++)
    {
        memset(global_cost_matrix[i], 0, sizeof(global_cost_matrix[i]));
    }
}

void MinCostFlowC::init_cost_matrix()
{
    for(int i = 0; i < this->node_num; i++)
    {
        memset(this->cost_matrix[i], 0, sizeof(int) * this->node_num);
    }
}

void MinCostFlowC::add_edge(int src, int des, int cap, int cost)
{

	edge_des[tol] = des;
	edge_next[tol] = head[src];
	edge_ori_cap[tol] = cap;
	edge_cap[tol] = cap;
	edge_cost[tol] = cost;
	edge_ori_cost[tol] = cost;
	edge_flow[tol] = 0;
    head[src] = tol++;

	edge_des[tol] = src;
	edge_next[tol] = head[des];
	edge_ori_cap[tol] = 0;
	edge_cap[tol] = 0;
	edge_cost[tol] = -cost;
	edge_ori_cost[tol] = -cost;
	edge_flow[tol] = 0;
    head[des] = tol++;

}

void MinCostFlowC::init(vector<bool>& servers)
{
        // 参数重置
    this->min_cost = INF;
    this->max_flow = 0;
    for (int i = 0; i < this->tol; ++i)
    {
        edge_cap[i] = edge_ori_cap[i];
        edge_cost[i] = edge_ori_cost[i];
        edge_flow[i] = 0;
    }
    this->tol = (this->edge_num + this->graph->cus_node_num) * 2;

    for (int edge_id = head[source]; edge_id != -1; edge_id = edge_next[edge_id])
    {
        int des = edge_des[edge_id];
        if (head[des] != -1)
        {
			head[des] = edge_next[head[des]];
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

    this->init_cost_matrix();
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
    for(int i = head[src]; i != -1; i = edge_next[i])
        if(edge_cap[i] && !edge_cost[i] && !vis[edge_des[i]])
        {
            int delta = zkw_aug(edge_des[i], tmp < edge_cap[i] ? tmp : edge_cap[i]);
			edge_cap[i] -= delta;
            edge_flow[i] += delta;
            edge_cap[i^1] += delta;
            edge_flow[i^1] -= delta;
            tmp -= delta;
            if(!tmp) return flow;
        }
    return flow - tmp;
}

int MinCostFlowC::zkw_aug_record_cost(int src, int flow)
{
    if(src == this->sink)
    {
        //cout<<first_node<<" "<<last_node<<endl;
        this->cost_matrix[this->first_node][last_node] = this->min_delta_cost * flow;
        this->min_cost += this->min_delta_cost * flow;
        this->max_flow += flow;
        return flow;
    }
    vis[src] = 1;
    int tmp = flow;

	for(int i = head[src]; i != -1; i = edge_next[i])
        if(edge_cap[i] && !edge_cost[i] && !vis[edge_des[i]])
        {
			if(src == this->source)
                this->first_node = edge_des[i];
            if(edge_des[i] == this->sink)
                this->last_node = src;

            int delta = zkw_aug_record_cost(edge_des[i], tmp < edge_cap[i] ? tmp : edge_cap[i]);
			edge_cap[i] -= delta;
            edge_flow[i] += delta;
            edge_cap[i^1] += delta;
            edge_flow[i^1] -= delta;
            tmp -= delta;
            if(!tmp) return flow;
        }
    return flow - tmp;
}

int MinCostFlowC::zkw_aug_record_all_cost(int src, int flow, int cost)
{
    if(src == this->sink)
    {
        this->min_cost += this->min_delta_cost * flow;
        this->max_flow += flow;
        return flow;
    }
    vis[src] = 1;
    int tmp = flow;

	for(int i = head[src]; i != -1; i = edge_next[i])
	{
        if(edge_cap[i] && !edge_cost[i] && !vis[edge_des[i]])
        {

			if(src == this->source)
                this->first_node = edge_des[i];
            if(edge_des[i] == this->sink)
                this->last_node = src;

            int delta = zkw_aug_record_all_cost(edge_des[i], tmp < edge_cap[i] ? tmp : edge_cap[i], cost + edge_ori_cost[i]);
            if(src != source)
            {
                //cout<<delta<<" "<<cost<<endl;
                global_cost_matrix[first_node][src] += delta * cost;
            }

			edge_cap[i] -= delta;
            edge_flow[i] += delta;
            edge_cap[i^1] += delta;
            edge_flow[i^1] -= delta;
            tmp -= delta;
            if(!tmp) return flow;
        }
	}
    return flow - tmp;
}

bool MinCostFlowC::run_zkw_mcf_record_all_cost()
{
    this->min_cost = 0;
    this->max_flow = 0;
    this->min_delta_cost = 0;
    init_global_cost_matrix();

    do
    {
        do
        {
            memset(vis, 0, sizeof(vis));
        } while(zkw_aug_record_all_cost(this->source, INF, 0));
    } while(zkw_modlabel());

    return this->max_flow == this->sum_demand;
}

bool MinCostFlowC::zkw_modlabel()
{
    int delta = INF;
    for(int u = 0; u < this->node_num; u++)
    {
        if(!vis[u]) continue;
        for(int i = head[u]; i != -1; i = edge_next[i])
            if(edge_cap[i] && !vis[edge_des[i]] && edge_cost[i] < delta)
                delta = edge_cost[i];
    }

    if(delta == INF) return false;
    for(int u = 0; u < this->node_num; u++)
    {
        if(!vis[u]) continue;
        for(int i = head[u]; i != -1; i = edge_next[i])
            edge_cost[i] -= delta, edge_cost[i^1] += delta;
    }
    this->min_delta_cost += delta;
    return true;
}

bool MinCostFlowC::run_zkw_mcf()
{
    this->min_cost = 0;
    this->max_flow = 0;
    this->min_delta_cost = 0;
    //delta_costs[delta_cnt++] = min_delta_cost;

    do
    {
        do
        {
            memset(vis, 0, sizeof(vis));
        } while(zkw_aug(this->source, INF));
    } while(zkw_modlabel());

    return this->max_flow == this->sum_demand;
}

bool MinCostFlowC::run_zkw_mcf_record_cost()
{
    this->min_cost = 0;
    this->max_flow = 0;
    this->min_delta_cost = 0;
    do
    {
        do
        {
            memset(vis, 0, sizeof(vis));
        } while(zkw_aug_record_cost(this->source, INF));
    } while(zkw_modlabel());

    return this->max_flow == this->sum_demand;
}

void MinCostFlowC::set_can_supply(vector<bool>& node_array)
{
    int des;
    for(int i = head[sink]; i != -1; i = edge_next[i])
    {
        des = edge_des[i];
        if(edge_cap[i] == 0)
        {
            node_array[des] = true;
        }
    }
}

int MinCostFlowC::is_all_satisfied()
{
    int max_demand = 0;
    int cus_id = -1;
    for (int i = head[sink]; i != -1; i = edge_next[i])
    {
        //cout<<this->all_edges[i]->flow << " "<<this->all_edges[i^1]->cap<<endl;
        if(edge_flow[i] != -edge_ori_cap[i^1])
        {
            if(edge_ori_cap[i^1] > max_demand)
            {
                max_demand = edge_ori_cap[i^1];
                cus_id = edge_des[i];
            }
        }
    }
    return cus_id;
}

MinCostFlowC::~MinCostFlowC()
{
    for(int i = 0; i < this->node_num; i++)
    {
        delete this->cost_matrix[i];
    }
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
        for (int i = pre[sink]; i != -1; i = pre[edge_des[i^1]])
        {
            if (path_bandwidth > edge_flow[i])
            {
                path_bandwidth = edge_flow[i];
            }
        }
        path.push(path_bandwidth);  // 路径带宽
        int id_net_node = edge_des[pre[sink]^1];
        Edge* out_edge = this->graph->node_out_edges[id_net_node][MAX_OUT_EDGE];
        if (out_edge)
            path.push(out_edge->des);  // 消费节点的独立id
        else
            cout << "error! not a valid demand node" << endl;
        for (int i = pre[sink]; i != -1; i = pre[edge_des[i^1]] )
        {
            if (edge_des[i^1] != source)
            {
                path.push(edge_des[i^1]);
            }
            edge_flow[i] -= path_bandwidth;
        }
        flow += path_bandwidth;
        paths.push_back(path);
    }
    //cout << "BFS flow: " << flow << endl;
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
        for (int i = head[u]; i != -1; i = edge_next[i])
        {
            int v = edge_des[i];
            if (edge_flow[i] > 0)
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

void MinCostFlowC::construct_neighbor_flows(vector<bool>& servers, vector<vector<NeighborFlow>>& neighbor_flows)
{
    int real_node_num = servers.size(), des;
    memset(neighbor_cnts, 0, sizeof(neighbor_cnts));
    for(int i = 0; i < servers.size(); i++)
    {
        if(!servers[i]) continue;
        for(int j = head[i]; j != -1; j = edge_next[j])
        {
            if(edge_ori_cost[j] < 0) continue;
            des = edge_des[j];
            if(des >= real_node_num) continue;
            neighbor_flows[i][neighbor_cnts[i]].node_id = des;
            neighbor_flows[i][neighbor_cnts[i]].flow = edge_flow[j];
            neighbor_flows[i][neighbor_cnts[i]].cost = edge_ori_cost[j] * edge_flow[j];
            neighbor_cnts[i]++;
            //cout<<i<<" "<<des<<" "<<edge_ori_cost[j]<<" "<<edge_flow[j]<<" "<<edge_ori_cost[j] * edge_flow[j]<<endl;

        }
    }
}

void MinCostFlowC::zkw_increase_one_server(vector<bool>& servers, int server_id)
{
    int des;
    add_edge(source, server_id, INF, 0);
    int cnt = 0;
    while(find_a_negtive_circle_by_priority_queue())
    {
        cicle_cnt++;
        //cout<<"server id: "<<server_id<<endl;
        /*cout<<dis[source]<<endl;
        for(int i = 0; i < node_num; i++)
        {
            cout<<i<<" "<<edge_des[pre[i]^1]<<endl;
        }*/
        memset(vis, 0, sizeof(bool) * node_num);
        vis[source] = true;
        for (int i = pre[source];; i = pre[edge_des[i^1]])
        {
            des = edge_des[i^1];
            if(vis[des]) break;
            vis[des] = true;
        }
        //vis[source] = true;
        int aug_flow = INF, start = des;
        memset(vis, 0, sizeof(bool) * node_num);
        vis[start] = true;
        for (int i = pre[start];; i = pre[edge_des[i^1]])
        {
            int remain_cap = edge_ori_cap[i] - edge_flow[i];
            if (aug_flow > remain_cap)
            {
                aug_flow = remain_cap;
            }
            des = edge_des[i^1];
            if(vis[des]) break;
            vis[des] = true;

        }

        //cout<<endl;
        //cout<<"aug flow: "<<aug_flow<<endl;
        memset(vis, 0, sizeof(bool) * node_num);
        vis[start] = true;
        for (int i = pre[start];; i = pre[edge_des[i^1]])
        {
            edge_flow[i] += aug_flow;
            edge_flow[i^1] -= aug_flow;
            min_cost += edge_ori_cost[i] * aug_flow;

            des = edge_des[i^1];
            if(vis[des]) break;
            vis[des] = true;
        }
        //max_flow += aug_flow;
        //cout<<dis[source]<<endl;
        //if(cnt++ > 5) break;
    }
    //run_min_cost_flow();
    //cout<<"min cost circle: "<<min_cost<<endl;
    //cout<<"max flow: "<<max_flow<<endl;
}

bool MinCostFlowC::run_min_cost_flow()                      // 计算最小费用流主函数
{
    //int flow = 0, cost = 0;
    while (spfa())
    {
        cout << "lalala... ";
        int aug_flow = INF;
        for (int i = pre[sink]; i != -1; i = pre[edge_des[i^1]])
        {
            int remain_cap = edge_ori_cap[i] - edge_flow[i];
            if (aug_flow > remain_cap)
            {
                aug_flow = remain_cap;
            }
        }
        for (int i = pre[sink]; i != -1; i = pre[edge_des[i^1]])
        {
            edge_flow[i] += aug_flow;
            edge_flow[i^1] -= aug_flow;
            min_cost += edge_ori_cost[i] * aug_flow;
        }
        max_flow += aug_flow;
    }
    //this->min_cost = cost;
    //this->max_flow = flow;
    //return flow == this->sum_demand;
    return true;
}

bool MinCostFlowC::find_a_negtive_circle()
{
    queue<int> q;
    for (int i = 0; i < this->node_num; ++i)
    {
        dis[i] = INF;
        vis[i] = false;
        pre[i] = -1;
    }

    memset(push_time, 0, sizeof(int) * node_num);
    dis[source] = 0;
    vis[source] = true;
    q.push(source);
    push_time[source]++;
    while (!q.empty())
    {
        int u = q.front();
        q.pop();
        vis[u] = false;
        for (int i = head[u]; i != -1; i = edge_next[i])
        {
            int v = edge_des[i];
            if (edge_ori_cap[i] > edge_flow[i] && dis[v] > dis[u] + edge_ori_cost[i])
            {
                dis[v] = dis[u] + edge_ori_cost[i];
                pre[v] = i;
                if (!vis[v])
                {
                    vis[v] = true;
                    q.push(v);
                    push_time[v]++;
                    //if(push_time[v] >= node_num)   return true;
                }
                if(v == source) return true;
            }
        }
    }
    return false;
}

bool MinCostFlowC::find_a_negtive_circle_by_priority_queue()
{
    for (int i = 0; i < this->node_num; ++i)
    {
        dis[i] = INF;
        pre[i] = -1;
    }

    dis[source] = 0;
    priority_queue<pair<int, int> > q;

    q.push(make_pair(0, source));
    while (!q.empty())
    {
        int u = q.top().second, d = - q.top().first;
        q.pop();
        if(dis[u] != d) continue;
        for (int i = head[u]; i != -1; i = edge_next[i])
        {
            int v = edge_des[i];
            if (edge_ori_cap[i] > edge_flow[i] && dis[v] > dis[u] + edge_ori_cost[i])
            {
                dis[v] = dis[u] + edge_ori_cost[i];
                pre[v] = i;
                q.push(make_pair(-dis[v], v));
                if(v == source) return true;
            }
        }
    }
    return false;
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
        for (int i = head[u]; i != -1; i = edge_next[i])
        {
            int v = edge_des[i];
            if (edge_ori_cap[i] > edge_flow[i] && dis[v] > dis[u] + edge_ori_cost[i])
            {
                dis[v] = dis[u] + edge_ori_cost[i];
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


#endif  // _MIN_COST_FLOW_H
