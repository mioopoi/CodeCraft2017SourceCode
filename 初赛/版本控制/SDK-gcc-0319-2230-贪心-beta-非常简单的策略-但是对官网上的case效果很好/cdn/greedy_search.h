#ifndef __GREEDY_SEARCH_H_
#define __GREEDY_SEARCH_H_

#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include <stack>
#include <queue>

#include "graph.h"
#include "greedy_helper.h"
#include "globals.h"
#include "min_cost_flow_c.h"

using namespace std;


class GreedySearch
{
public:
    int node_num;
    int customer_num;
    Graph* graph;

    vector<Customer*> customers;

    vector<bool> servers;
    int total_server;
    int min_cost;

    MinCostFlowC* mcf_machine;

    GreedySearch(Graph* graph);
    ~GreedySearch();
    void init_customers();

    void greedy_search_algorithm();
    Customer* choose_first_server();
    Customer* choose_thirstiest_customer();

    /*alpha*/
    void greed_search_algorithm_alpha();
    int choose_an_cost_most_node();
    int increase_servers();
    int choose_an_cost_least_servers();
    void decrease_servers();
    bool remove_server(int server_id);
    void add_server(int server_id);

    /*beta*/
    void greed_search_algorithm_beta();
};

GreedySearch::GreedySearch(Graph* graph)
{
    this->graph = graph;
    this->node_num = graph->net_node_num;
    this->customer_num = graph->cus_node_num;

    mcf_machine = new MinCostFlowC(graph);

    this->customers = vector<Customer*>(this->node_num, NULL);
    this->init_customers();

    this->min_cost = 0;
    this->total_server = 0;
    this->servers = vector<bool>(this->node_num, false);
}

void GreedySearch::init_customers()
{
    Cus_Node* cur;
    for(int i = 0; i < this->customer_num; i++)
    {
        cur = graph->cus_nodes[i];
        this->customers[cur->des] = new Customer(cur->des, cur->need);
    }

}

void GreedySearch::greedy_search_algorithm()
{
    Customer* server;
    server = choose_first_server();

    while(true)
    {
        this->total_server++;
        //server->print_customer();
        this->servers[server->cus_id] = true;
        this->mcf_machine->init(this->servers);
        //this->mcf_machine->run_min_cost_flow();
        this->mcf_machine->run_zkw_mcf();
        server = choose_thirstiest_customer();
        if(server == NULL) break;
        //return;
    }

    this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
    cout<<"min_cost after greedy_search_algorithm: "<<this->min_cost<<endl;
}

int GreedySearch::increase_servers()
{
    int cur_min_cost = this->min_cost, id;
    while(true)
    {
        id = this->choose_an_cost_most_node();
        if(id == -1) break;

        this->total_server++;
        //this->customers[id]->print_customer();
        this->servers[id] = true;
        this->mcf_machine->init(this->servers);
        //this->mcf_machine->run_min_cost_flow();
        this->mcf_machine->run_zkw_mcf();

        this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
        //cout<<"current: "<<id<<" "<<this->min_cost<<endl;
        if(this->min_cost >= cur_min_cost)
        {
            this->total_server--;
            this->servers[id] = false;
            this->mcf_machine->init(this->servers);
            //this->mcf_machine->run_min_cost_flow();
            this->mcf_machine->run_zkw_mcf();
            this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
            break;
        }
        cur_min_cost = this->min_cost;
    }

    cout<<"min_cost after increase servers: "<<this->min_cost<<endl;
    return id;
}

void GreedySearch::decrease_servers()
{
    int cur_min_cost = this->min_cost;
    bool feasible;
    while(true)
    {
        int id = this->choose_an_cost_least_servers();
        if(id == -1) break;
        this->customers[id]->print_customer();
        this->total_server--;
        this->servers[id] = false;
        this->mcf_machine->init(this->servers);
        //this->mcf_machine->run_min_cost_flow();
        feasible = this->mcf_machine->run_zkw_mcf();
        if(feasible)
        {
            this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
            if(this->min_cost < cur_min_cost)
                continue;
        }


        this->total_server++;
        this->servers[id] = true;
        this->mcf_machine->init(this->servers);
        //this->mcf_machine->run_min_cost_flow();
        this->mcf_machine->run_zkw_mcf();
        this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
        break;
    }

    cout<<"min_cost after decrease servers: "<<this->min_cost<<endl;
}

void GreedySearch::greed_search_algorithm_alpha()
{
    this->greedy_search_algorithm();
    this->increase_servers();
    //this->decrease_servers();

}

int GreedySearch::choose_an_cost_most_node()
{
    int max_demand = 0;
    int id = -1;
    for(int i = 0; i < this->node_num; i++)
    {
        if(this->servers[i] || this->customers[i] == NULL) continue;
        if(this->customers[i]->demand > max_demand)
        {
            id = i;
            max_demand = this->customers[i]->demand;
        }
    }
    return id;
}

int GreedySearch::choose_an_cost_least_servers()
{
    int min_demand = INF;
    int id = -1;
    for(int i = 0; i < this->node_num; i++)
    {
        if(this->servers[i])
        {
            if(this->customers[i]->demand < min_demand)
            {
                id = i;
                min_demand = this->customers[i]->demand;
            }
        }

    }
    return id;
}

Customer* GreedySearch::choose_first_server()
{
    Customer* cur;
    int max_demand = 0;

    for(int i = 0; i < this->node_num; i++)
    {
        if(this->customers[i] == NULL) continue;
        if(max_demand < this->customers[i]->demand)
        {
            max_demand = this->customers[i]->demand;
            cur = this->customers[i];
        }
    }

    return cur;
}

Customer* GreedySearch::choose_thirstiest_customer()
{
    int cus_id = this->mcf_machine->is_all_satisfied();
    if(cus_id == -1) return NULL;
    return this->customers[cus_id];

}

GreedySearch::~GreedySearch()
{
    for(int i = 0; i < this->customer_num; i++)
        delete this->customers[i];
    delete mcf_machine;
}

/*void GreedySearch::greed_search_algorithm_beta()
{
    priority_queue<Customer*, vector<Customer*>, customer_demand_cmp> pq;

    for(int i = 0; i < this->node_num; i++)
    {
        if(customers[i] == NULL) continue;
        pq.push(customers[i]);
    }

    stack<int> id_stack;
    Customer* server, *tmp;
    int tmp_min_cost = INF;
    while(!pq.empty())
    {
        server = pq.top();
        id_stack.push(server->cus_id);
        //server->print_customer();
        pq.pop();
        this->add_server(server->cus_id);

        tmp = choose_thirstiest_customer();
        if(tmp == NULL)
        {
            this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
            if(this->min_cost >= tmp_min_cost)
            {
                this->total_server--;
                this->min_cost = tmp_min_cost;
                this->servers[server->cus_id] = false;
                //remove_server(server->cus_id);
            }
            else
                tmp_min_cost = this->min_cost;
        }

    }

    this->mcf_machine->init(this->servers);
    //this->mcf_machine->run_min_cost_flow();
    this->mcf_machine->run_zkw_mcf();
    this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
    cout<<"min_cost after greedy_search_algorithm beta: "<<this->min_cost<<endl;

    int id;
    bool is_feasible;
    tmp_min_cost = this->min_cost;
    //id_stack.pop();
    while(!id_stack.empty())
    {
        id = id_stack.top();
        id_stack.pop();
        if(!servers[id]) continue;
        is_feasible = remove_server(id);
        if(this->min_cost >= tmp_min_cost || !is_feasible)
        {
            this->total_server++;
            this->min_cost = tmp_min_cost;
            this->servers[id] = true;
        }
        else
        {
            tmp_min_cost = this->min_cost;
        }

    }

    this->mcf_machine->init(this->servers);
    //this->mcf_machine->run_min_cost_flow();
    this->mcf_machine->run_zkw_mcf();
    this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
    cout<<"min_cost after greedy_search_algorithm beta: "<<this->min_cost<<endl;
}*/

void GreedySearch::greed_search_algorithm_beta()
{
    priority_queue<Customer*, vector<Customer*>, customer_demand_cmp> pq;

    for(int i = 0; i < this->node_num; i++)
    {
        if(customers[i] == NULL) continue;
        pq.push(customers[i]);
    }

    queue<int> id_queue;
    Customer* server, *tmp;
    int tmp_min_cost = INF;
    while(!pq.empty())
    {
        server = pq.top();
        id_queue.push(server->cus_id);
        //server->print_customer();
        pq.pop();
        this->add_server(server->cus_id);

        tmp = choose_thirstiest_customer();
        if(tmp == NULL)
        {
            this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
            if(this->min_cost >= tmp_min_cost)
            {
                this->total_server--;
                this->min_cost = tmp_min_cost;
                this->servers[server->cus_id] = false;
                //remove_server(server->cus_id);
            }
            else
                tmp_min_cost = this->min_cost;
        }

    }

    this->mcf_machine->init(this->servers);
    //this->mcf_machine->run_min_cost_flow();
    this->mcf_machine->run_zkw_mcf();
    this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
    cout<<"min_cost after greedy_search_algorithm beta: "<<this->min_cost<<endl;

    int id;
    bool is_feasible;
    tmp_min_cost = this->min_cost;
    //id_stack.pop();
    while(!id_queue.empty())
    {
        id = id_queue.front();
        id_queue.pop();
        if(!servers[id]) continue;
        is_feasible = remove_server(id);
        if(this->min_cost >= tmp_min_cost || !is_feasible)
        {
            this->total_server++;
            this->min_cost = tmp_min_cost;
            this->servers[id] = true;
        }
        else
        {
            tmp_min_cost = this->min_cost;
        }

    }

    this->mcf_machine->init(this->servers);
    //this->mcf_machine->run_min_cost_flow();
    this->mcf_machine->run_zkw_mcf();
    this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
    cout<<"min_cost after greedy_search_algorithm beta: "<<this->min_cost<<endl;
}

bool GreedySearch::remove_server(int server_id)
{
    this->total_server--;
    this->servers[server_id] = false;
    this->mcf_machine->init(this->servers);
    //this->mcf_machine->run_min_cost_flow();
    bool is_feasible = this->mcf_machine->run_zkw_mcf();
    this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
    return is_feasible;
}

void GreedySearch::add_server(int server_id)
{
    this->total_server++;
    this->servers[server_id] = true;
    this->mcf_machine->init(this->servers);
    //this->mcf_machine->run_min_cost_flow();
    this->mcf_machine->run_zkw_mcf();
    this->min_cost = this->total_server * this->graph->net_fee + this->mcf_machine->min_cost;
}

#endif
