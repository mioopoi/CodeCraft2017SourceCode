#ifndef __OPT_SEARCH_HELPER_H__
#define __OPT_SEARCH_HELPER_H__

struct Candidate
{
    int node_id;
    int demand;
    int node_cost;
    int max_flow;
    int cus_num;
    double overflow_cost;
    int pass_cost;
    int pass_flow;
    int server_out_flow;
    int min_cost;

    Candidate(int node_id = -1, int demand = -1, int node_cost = 100000) : node_id(node_id),    demand(demand), node_cost(node_cost)
    {
        overflow_cost = 0.0;
        pass_cost = 0;
        server_out_flow = 0;
    }
    void* operator new(size_t, void* p)    {return p;}
};

struct candidate_demand_cmp_greater
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        if(lhs->node_id == -1 || rhs->node_id == -1)
            return lhs->node_id > rhs->node_id;
        if(lhs->demand != rhs->demand)
            return lhs->demand > rhs->demand;
        return lhs->node_cost < rhs->node_cost;
    }
};

struct candidate_flow_cmp_greater
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        if(lhs->node_id == -1 || rhs->node_id == -1)
            return lhs->node_id > rhs->node_id;

        if(lhs->max_flow != rhs->max_flow)
            return lhs->max_flow > rhs->max_flow;
        if(lhs->cus_num != rhs->cus_num)
            return lhs->cus_num > rhs->cus_num;
        return lhs->node_cost < rhs->node_cost;
    }
};

struct candidate_overflow_cmp_greater
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        if(lhs->node_id == -1 || rhs->node_id == -1)
            return lhs->node_id > rhs->node_id;
        return lhs->overflow_cost > rhs->overflow_cost;
    }
};

struct candidate_pass_cost_cmp_greater
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        if(lhs->node_id == -1 || rhs->node_id == -1)
            return lhs->node_id > rhs->node_id;
        return lhs->pass_cost > rhs->pass_cost;
    }
};

struct candidate_pass_flow_cmp_greater
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        if(lhs->node_id == -1 || rhs->node_id == -1)
            return lhs->node_id > rhs->node_id;
        return lhs->pass_flow > rhs->pass_flow;
    }
};

struct candidate_cus_num_cmp_greater
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        if(lhs->node_id == -1 || rhs->node_id == -1)
            return lhs->node_id > rhs->node_id;
        return lhs->cus_num > rhs->cus_num;
    }
};

struct candidate_server_out_flow_cmp_less
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        if(lhs->node_id == -1 || rhs->node_id == -1)
            return lhs->node_id > rhs->node_id;
        return lhs->server_out_flow < rhs->server_out_flow;
    }
};

struct ServerCost
{
    int server_id;
    int customer_id;
    int cost;

    ServerCost(int server_id = -1, int customer_id = -1, int cost = -1) : server_id(server_id), customer_id(customer_id), cost(cost) {}
};

struct server_cost_cmp_greater
{
    bool operator()(const ServerCost& lhs, const ServerCost& rhs)
    {
        return lhs.cost > rhs.cost;
    }
};

struct server_cost_cmp_less
{
    bool operator()(const ServerCost& lhs, const ServerCost& rhs)
    {
        return lhs.cost < rhs.cost;
    }
};


struct server_min_cost_cmp_less
{
    bool operator()(Candidate* lhs, Candidate* rhs)
    {
        return lhs->min_cost < rhs->min_cost;
    }
};

#endif
