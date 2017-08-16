#ifndef __GREEDY_SEARCH_ALPHA_H_
#define __GREEDY_SEARCH_ALPHA_H_

#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <stack>
#include <ctime>

#include "graph.h"
#include "greedy_helper.h"
#include "globals.h"
#include "min_cost_flow_c.h"

class GreedySearchAlpha
{
public:
    int node_num;
    int customer_num;
    Graph* graph;

    vector<Customer*> customers;
    vector<bool> servers;
    vector<AveFlowCost*> ave_costs;
    vector<int> candidates;
    int global_total_server;
    int global_min_cost;

    MinCostFlowC* mcf_machine;


    GreedySearchAlpha(Graph* graph);
    ~GreedySearchAlpha();

    void init_customers();
    void init_ave_costs();
    bool run_mcf(int which = 0);
    bool add_a_server(int server_id, int type = 0);
    bool remove_a_server(int server_id);


    int mcf_cnt = 0;
    vector<vector<bool>> can_supply;

    void construct_candidates_zero();
    void greedy_search_zero();
    bool decrease_servers_zero();
    bool increase_servers_zero();
    void show_supply_detail();
    bool remove_server_after_increase(int server_id);
    bool exchange_servers_zero();
    bool exchange_servers_one();
    bool exchange_servers_two();
    void exchange_servers_three();

    void construct_server_customer_costs(int type);
    void init_server_customer_costs();
    vector<ServerCost> customer_costs;
    vector<ServerCost> server_costs;

    void construct_candidates_one();

    int topo_lv;

    vector<vector<bool>> is_neighbor;
    void init_is_neighbor();

    /*topo lv 2*/
    void greedy_search_one();
    void greedy_search_two();
    void construct_exchange_node_queue_zero(priority_queue<ExchangeNode*, vector<ExchangeNode*>, exchange_cost_cmp_less>& pq);
    void topo_stratedy();


};

GreedySearchAlpha::GreedySearchAlpha(Graph* graph)
{
    this->graph = graph;
    this->node_num = graph->net_node_num;
    this->customer_num = graph->cus_node_num;

    mcf_machine = new MinCostFlowC(graph);

    this->customers = vector<Customer*>(this->node_num, NULL);
    this->init_customers();

    this->global_min_cost = INF;
    this->global_total_server = 0;
    this->servers = vector<bool>(this->node_num, false);
    this->can_supply = vector<vector<bool>>(this->node_num, vector<bool>(node_num, false));
    this->ave_costs = vector<AveFlowCost*>(this->node_num, NULL);

    this->mcf_cnt = 0;
    //this->init_ave_costs();

    this->customer_costs = vector<ServerCost>(this->node_num);
    this->server_costs = vector<ServerCost>(this->node_num);

    is_neighbor = vector<vector<bool>>(node_num, vector<bool>(node_num, false));
    init_is_neighbor();

    if(node_num <= 200) topo_lv = 0;
    else if(node_num <= 300) topo_lv = 1;
    else topo_lv = 2;
}

void GreedySearchAlpha::init_is_neighbor()
{
    int src, des;
    for(int i = 0; i < graph->edge_num; i++)
    {
        src = graph->net_edges[i]->src;
        des = graph->net_edges[i]->des;
        is_neighbor[src][des] = true;
        is_neighbor[des][src] = true;
    }
}


void GreedySearchAlpha::init_customers()
{
    Cus_Node* cur;
    for(int i = 0; i < this->customer_num; i++)
    {
        cur = graph->cus_nodes[i];
        this->customers[cur->des] = new Customer(cur->des, cur->need);
    }
}

void GreedySearchAlpha::init_ave_costs()
{
    int min_cost;
    int max_flow;
    for(int i = 0; i < this->node_num; i++)
    {
        this->servers[i] = true;
        this->mcf_machine->init(this->servers);
        this->mcf_cnt++;
        this->mcf_machine->run_zkw_mcf();
        min_cost = this->mcf_machine->min_cost;
        max_flow = this->mcf_machine->max_flow;
        mcf_machine->set_can_supply(this->can_supply[i]);
        this->ave_costs[i] = new AveFlowCost(i, min_cost / (double) max_flow);
        this->servers[i] = false;
        //cout << this->ave_costs[i]->node_id<<" "<<this->ave_costs[i]->ave_cost<<endl;
    }

    sort(this->ave_costs.begin(), this->ave_costs.end(), ave_cost_cmp_less());
    for(int i = 0; i < this->node_num; i++)
    {
        //cout << this->ave_costs[i]->node_id<<" "<<this->ave_costs[i]->ave_cost<<endl;
    }
}

bool GreedySearchAlpha::run_mcf(int which)
{
    this->mcf_cnt++;
    this->mcf_machine->init(this->servers);
    bool is_feasible;
    if(which == 0)
        is_feasible = this->mcf_machine->run_zkw_mcf();
    else
        is_feasible = this->mcf_machine->run_zkw_mcf_record_cost();
    this->global_min_cost = this->global_total_server * this->graph->net_fee + this->mcf_machine->min_cost;
    return is_feasible;
}

bool GreedySearchAlpha::add_a_server(int server_id, int type)
{
    this->servers[server_id] = true;
    this->global_total_server++;
    return this->run_mcf(type);
}

bool GreedySearchAlpha::remove_a_server(int server_id)
{
    this->servers[server_id] = false;
    this->global_total_server--;
    return this->run_mcf();
}

GreedySearchAlpha::~GreedySearchAlpha()
{
    for(int i = 0; i < this->node_num; i++)
    {
        delete this->customers[i];
        delete this->ave_costs[i];
    }

    delete mcf_machine;
}

void GreedySearchAlpha::init_server_customer_costs()
{
    for(int i = 0; i < this->node_num; i++)
    {
        this->customer_costs[i].node_id = i;
        this->customer_costs[i].cost = 0;
        this->server_costs[i].node_id = i;
        this->server_costs[i].cost = 0;
    }
}

void GreedySearchAlpha::construct_server_customer_costs(int type)
{
    this->init_server_customer_costs();
    for(int i = 0; i < servers.size(); i++)
    {
        if(!servers[i]) continue;
        for(int j = 0; j < this->node_num; j++)
        {
            if(this->mcf_machine->cost_matrix[i][j] != 0)
            {
                if(type >= 0)
                    this->customer_costs[j].cost +=  this->mcf_machine->cost_matrix[i][j];
                if(type >= 1)
                    this->server_costs[i].cost +=  this->mcf_machine->cost_matrix[i][j];
            }
        }
    }

    if(type >= 0)
        sort(customer_costs.begin(), customer_costs.end(), server_cost_cmp_greater());
    if(type >= 1)
        sort(server_costs.begin(), server_costs.end(), server_cost_cmp_greater());

    for(int i = 0; i < this->node_num; i++)
    {
        if(type == 2)
            cout<<customer_costs[i].node_id<<" "<<customer_costs[i].cost<<endl;
            //cout<<server_costs[i].node_id<<" "<<server_costs[i].cost<<endl;
    }
}

void GreedySearchAlpha::topo_stratedy()
{

    if(topo_lv == 0)
    {
        greedy_search_zero();
    }
    else if(topo_lv == 1)
    {
        greedy_search_zero();
    }
    else if(topo_lv == 2)
    {
        greedy_search_one();
    }

    this->show_supply_detail();
    //this->run_mcf(1);
    //construct_server_customer_costs(2);
}

void GreedySearchAlpha::greedy_search_zero()
{
    this->init_ave_costs();
    construct_candidates_zero();
    for(int i = 0; i < candidates.size(); i++)
    {
        servers[candidates[i]] = true;
        global_total_server++;
    }

    reverse(candidates.begin(), candidates.end());
    global_min_cost = graph->net_fee * global_total_server;

    int tmp_cost = INF, cnt = 0;

    int fail0 = INF, fail1 = INF;
    while(true)
    {
        decrease_servers_zero();
        increase_servers_zero();
        /*if(cnt == 1)
        {
            this->construct_candidates_one();
        }*/
        if(global_min_cost == fail0) break;
        if(!exchange_servers_two());
        {
            fail0 = global_min_cost;
        }

        if(global_min_cost == fail1) break;
        if(!exchange_servers_one());
        {
            fail1 = global_min_cost;
        }

    }
    //decrease_servers_zero();
    cout<<"mcf cnt: "<<mcf_cnt<<endl;
    //this->show_supply_detail();
    //this->run_mcf(1);
    //construct_server_customer_costs(2);
}

void GreedySearchAlpha::greedy_search_one()
{
    this->init_ave_costs();
    construct_candidates_zero();
    for(int i = 0; i < candidates.size(); i++)
    {
        servers[candidates[i]] = true;
        global_total_server++;
    }

    reverse(candidates.begin(), candidates.end());
    global_min_cost = graph->net_fee * global_total_server;

    decrease_servers_zero();
    increase_servers_zero();
    exchange_servers_two();
    exchange_servers_one();
    decrease_servers_zero();
    increase_servers_zero();
}

void GreedySearchAlpha::greedy_search_two()
{
    this->init_ave_costs();
    construct_candidates_zero();
    for(int i = 0; i < candidates.size(); i++)
    {
        servers[candidates[i]] = true;
        global_total_server++;
    }

    reverse(candidates.begin(), candidates.end());
    global_min_cost = graph->net_fee * global_total_server;

    decrease_servers_zero();
    increase_servers_zero();
    decrease_servers_zero();
    exchange_servers_three();
    increase_servers_zero();
    decrease_servers_zero();
    //decrease_servers_zero();
    cout<<"mcf cnt: "<<mcf_cnt<<endl;
}

void GreedySearchAlpha::construct_candidates_one()
{
    int num0 = int(this->node_num * 0.3);
    int num = this->customer_num + num0;
    this->candidates = vector<int>(num);

    int index = 0, cnt = 0, node_id;
    for(int i = 0; i < this->node_num; i++)
    {
        //cout<<node_id<<" "<<this->ave_costs[i]->ave_cost<<endl;
        node_id = this->ave_costs[i]->node_id;
        if(this->customers[node_id] != NULL)
        {
            this->candidates[index++] = node_id;
        }
        else if(cnt < num0)
        {
            //cout<<node_id<<endl;
            this->candidates[index++] = node_id;
            cnt++;
        }
    }
}

void GreedySearchAlpha::show_supply_detail()
{
    /*this->servers[47] = false;
    this->servers[7] = false;

    this->servers[6] = true;
    this->servers[104] = true;
    //this->global_total_server--;*/

    bool is_feasible = this->run_mcf(1);
    cout<<is_feasible<<" "<<"min cost : "<<this->global_min_cost<<endl;

    for(int i = 0; i < servers.size(); i++)
    {
        if(!servers[i]) continue;
        for(int j = 0; j < this->node_num; j++)
        {
            if(this->mcf_machine->cost_matrix[i][j] != 0)
            {
                cout<<i<<" "<<j<<" "<<mcf_machine->cost_matrix[i][j]<<endl;
            }
        }
    }
}

void GreedySearchAlpha::construct_candidates_zero()
{
    this->candidates = vector<int>(this->customer_num);
    priority_queue<Customer*, vector<Customer*>, customer_demand_cmp_less> pq; //最大堆

    for(int i = 0; i < this->node_num; i++)
    {
        if(customers[i] == NULL) continue;
        pq.push(customers[i]);
    }

    Customer* cur;
    int cnt = 0;
    while(!pq.empty())
    {
        cur = pq.top();
        this->candidates[cnt++] = cur->cus_id;
        pq.pop();
    }

}

bool GreedySearchAlpha::decrease_servers_zero()
{
    int ori_cost = global_min_cost;
    int node_id, tmp_cost = global_min_cost;
    bool is_feasilble;
    for(int i = 0; i < this->candidates.size(); i++)
    {
        node_id = this->candidates[i];
        if(this->servers[node_id] == false) continue;
        is_feasilble = this->remove_a_server(node_id);
        if(is_feasilble && this->global_min_cost < tmp_cost)
        {
            tmp_cost = this->global_min_cost;
        }
        else
        {
            global_min_cost = tmp_cost;
            this->servers[node_id] = true;
            this->global_total_server++;
        }
    }
    cout<<"total cost after decrease server: "<<global_min_cost<<endl;

    if(global_min_cost < ori_cost) return true;
    return false;
    //this->run_mcf();
}

bool GreedySearchAlpha::increase_servers_zero()
{
    int ori_cost = global_min_cost;
    int node_id, tmp_cost = global_min_cost;
    bool is_feasilble;

    for(int i = 0; i < this->candidates.size(); i++)
    {
        node_id = this->candidates[i];
        if(this->servers[node_id]) continue;
        is_feasilble = this->add_a_server(node_id);
        if(this->global_min_cost < tmp_cost)
        {
            //this->remove_server_after_increase(node_id);
            tmp_cost = global_min_cost;
        }
        else
        {
            global_min_cost = tmp_cost;
            this->servers[node_id] = false;
            this->global_total_server--;
        }
    }

    cout<<"total cost after increase server: "<<global_min_cost<<endl;

    if(global_min_cost < ori_cost) return true;
    return false;
}

bool GreedySearchAlpha::remove_server_after_increase(int server_id)
{
    int tmp_cost;
    bool is_feasible;
    for(int i = 0; i < this->node_num; i++)
    {
        if(i == server_id || !servers[i] || !can_supply[server_id][i]) continue;
        tmp_cost = this->global_min_cost;
        is_feasible = this->remove_a_server(i);
        if(is_feasible && this->global_min_cost < tmp_cost)
        {
            //cout<<server_id<<" "<<i<<endl;
            tmp_cost = this->global_min_cost;
        }
        else
        {
            global_min_cost = tmp_cost;
            this->servers[i] = true;
            this->global_total_server++;
        }
    }
    return true;
}

void GreedySearchAlpha::construct_exchange_node_queue_zero(priority_queue<ExchangeNode*, vector<ExchangeNode*>, exchange_cost_cmp_less>& pq)
{
    int node_id, cnt = 0;

    for(int i = 0; i < candidates.size(); i++)
    {
        node_id = candidates[i];
        if(!servers[node_id]) continue;
        for(int j = 0; j < this->node_num; j++)
        {
            if(this->mcf_machine->cost_matrix[node_id][j] != 0)
            {
                pq.push(new ExchangeNode(node_id, j, mcf_machine->cost_matrix[node_id][j]));
            }
        }
    }
}

bool GreedySearchAlpha::exchange_servers_zero()
{
    this->run_mcf(1);
    this->construct_server_customer_costs(1);

    bool is_feasible;
    int tmp_cost = this->global_min_cost, node_id;

    double p = 1;
    if(topo_lv == 2) p = 0;
    int last = int(customer_costs.size() * p);
    for(int i = 0; i < last; i++)
    {
        node_id = this->customer_costs[i].node_id;
        if(customer_costs[i].cost == 0) break;
        for(int j = 0; j < node_num; j++)
        {
            if(!servers[j]) continue;
            servers[j] = false;
            servers[node_id] = true;

            is_feasible = this->run_mcf(1);
            if(is_feasible && this->global_min_cost < tmp_cost)
            {
                tmp_cost = this->global_min_cost;
                cout<<"total cost after exchange server zero: "<<global_min_cost<<endl;
                return true;
            }
            else
            {
                global_min_cost = tmp_cost;
                servers[j] = true;
                servers[node_id] = false;
            }
        }

    }

    cout<<"total cost after exchange server zero: "<<global_min_cost<<endl;
    return false;

}

bool GreedySearchAlpha::exchange_servers_one()
{
    this->run_mcf(1);
    this->construct_server_customer_costs(1);

    bool is_feasible, is_changed = false;
    int tmp_cost = this->global_min_cost, node_id;

    double p = 1;
    if(topo_lv == 2) p = 0.001;

    int cnt = 0;
    for(int i = node_num - 1; i >= 0; i--)
    {
        node_id = this->server_costs[i].node_id;

        if(!servers[node_id]) continue;
        if(server_costs[i].cost == 0 && !servers[node_id]) continue;

        for(int j = 0; j < node_num; j++)
        {
            if(servers[j]) continue;
            if(!is_neighbor[j][node_id]) continue;
            servers[j] = true;
            servers[node_id] = false;

            is_feasible = this->run_mcf(1);
            if(is_feasible && this->global_min_cost < tmp_cost)
            {
                //cout<<global_total_server<<" "<<j<<" "<<node_id<<endl;
                //cout<<can_supply[node_id][j]<<endl;
                tmp_cost = this->global_min_cost;
                //cout<<"total cost after exchange server one: "<<global_min_cost<<endl;
                is_changed  = true;
                break;
            }
            else
            {
                global_min_cost = tmp_cost;
                servers[j] = false;
                servers[node_id] = true;
            }
        }
    }

    cout<<"total cost after exchange server one: "<<global_min_cost<<endl;
    return is_changed;

}

bool GreedySearchAlpha::exchange_servers_two()
{
    this->run_mcf(1);
    int ch_out, ch_in, tmp_cost, size, cnt;
    bool is_feasible, is_changed;
    ExchangeNode* ex_node;
    is_changed = false;
    tmp_cost = this->global_min_cost;
    priority_queue<ExchangeNode*, vector<ExchangeNode*>, exchange_cost_cmp_less> pq; //最大堆
    this->construct_exchange_node_queue_zero(pq);
    size = int(pq.size() * 1);
    cnt = 0;

    while(!pq.empty() && cnt++ < size)
    {
        ex_node = pq.top();
        pq.pop();
        ch_out = ex_node->ch_out;
        ch_in = ex_node->ch_in;
        delete ex_node;
        if(!servers[ch_out] || servers[ch_in]) continue;
        this->servers[ch_out] = false;
        this->servers[ch_in] = true;
        is_feasible = this->run_mcf(1);
        if(is_feasible && this->global_min_cost < tmp_cost)
        {
            cout<<"exchange succeed: "<<global_min_cost<<endl;
            //cout<<percent<<" "<<cnt<<" "<<ch_out<<" "<<customers[ch_out]->demand<<" "<<ch_in<<" "<<customers[ch_in]->demand<<" "<<ex_node->cost<<" "<<endl;
            tmp_cost = this->global_min_cost;
            is_changed = true;
        }
        else
        {
            this->servers[ch_out] = true;
            this->servers[ch_in] = false;
            global_min_cost = tmp_cost;
        }
    }

    while(!pq.empty())
    {
        ex_node = pq.top();
        pq.pop();
        delete ex_node;
    }
    cout<<"total cost after exchange server two: "<<global_min_cost<<endl;
    return is_changed;

}

void GreedySearchAlpha::exchange_servers_three()
{
    this->run_mcf(1);
    int ch_out, ch_in, tmp_cost;
    bool is_feasible, is_changed, global_changed = false;
    ExchangeNode* ex_node;
    while(true)
    {
        is_changed = false;
        tmp_cost = this->global_min_cost;
        priority_queue<ExchangeNode*, vector<ExchangeNode*>, exchange_cost_cmp_less> pq; //最大堆
        this->construct_exchange_node_queue_zero(pq);
        while(!pq.empty())
        {
            ex_node = pq.top();
            pq.pop();
            ch_out = ex_node->ch_out;
            ch_in = ex_node->ch_in;
            //cout<<ch_out<<" "<<ch_in<<endl;
            delete ex_node;
            this->servers[ch_out] = false;
            this->servers[ch_in] = true;
            is_feasible = this->run_mcf(1);
            if(is_feasible && this->global_min_cost < tmp_cost)
            {
                global_changed = true;
                tmp_cost = this->global_min_cost;
                is_changed = true;
                break;
            }
            else
            {
                global_min_cost = tmp_cost;
                this->servers[ch_out] = true;
                this->servers[ch_in] = false;
            }
        }

        while(!pq.empty())
        {
            ex_node = pq.top();
            pq.pop();
            delete ex_node;
        }
        if(!is_changed) break;
    }

    if(!global_changed) return;

    cout<<"min cost after exchange three: "<<this->global_min_cost<<endl;
    //this->run_mcf();
}
#endif
