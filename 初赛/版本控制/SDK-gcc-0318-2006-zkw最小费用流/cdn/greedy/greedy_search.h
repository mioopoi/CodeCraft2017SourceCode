#ifndef __GREEDY_SEARCH_H_
#define __GREEDY_SEARCH_H_

#include <iostream>
#include <stdlib.h>
#include <algorithm>
#include "../graph.h"
#include "greedy_helper.h"
#include "../globals.h"
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
    cout<<"min_cost: "<<this->min_cost<<endl;
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
    Customer* cur;
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

#endif
