#ifndef __GREEDY_HELPER_H_
#define __GREEDY_HELPER_H_

/*类Customer定义开始*/
class Customer
{
public:
    int cus_id;
    int demand;

    Customer(int cus_id, int demand);
    void print_customer();

};

Customer::Customer(int cus_id, int demand)
{
    this->cus_id = cus_id;
    this->demand = demand;
}

void Customer::print_customer()
{
    cout<<"customer id: "<<cus_id<<" demand: "<<demand<<endl;
}

struct customer_demand_cmp_greater
{
    bool operator()(Customer* lhs, Customer* rhs)
    {
        return lhs->demand > rhs->demand;   //大于 最小堆，小于最大堆
    }
};

struct customer_demand_cmp_less
{
    bool operator()(Customer* lhs, Customer* rhs)
    {
        return lhs->demand < rhs->demand;
    }
};
/*类Customer定义开始*/


/*类ave_flow_cost定义开始*/
class AveFlowCost
{
public:
    int node_id;
    double ave_cost;

    AveFlowCost(int node_id, double ave_cost): node_id(node_id), ave_cost(ave_cost) {}
};

struct ave_cost_cmp_greater
{
    bool operator()(AveFlowCost* lhs, AveFlowCost* rhs)
    {
        return lhs->ave_cost > rhs->ave_cost;
    }
};

struct ave_cost_cmp_less
{
    bool operator()(AveFlowCost* lhs, AveFlowCost* rhs)
    {
        return lhs->ave_cost < rhs->ave_cost;
    }
};

class ExchangeNode
{
public:
    int ch_out;
    int ch_in;
    int cost;

    ExchangeNode(int ch_out, int ch_in, int cost) : ch_out(ch_out), ch_in(ch_in), cost(cost) {}
};

struct exchange_cost_cmp_less
{
    bool operator()(ExchangeNode* lhs, ExchangeNode* rhs)
    {
        return lhs->cost < rhs->cost;
    }
};

struct exchange_cost_cmp_greater
{
    bool operator()(ExchangeNode* lhs, ExchangeNode* rhs)
    {
        return lhs->cost > rhs->cost;
    }
};

class ServerCost
{
public:
    int node_id;
    int cost;

    ServerCost(int node_id = -1) : node_id(node_id), cost(0) {}
};

struct server_cost_cmp_greater
{
    bool operator()(const ServerCost& lhs, const ServerCost& rhs)
    {
        return lhs.cost > rhs.cost;
    }
};
#endif
