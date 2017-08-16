#ifndef __GREEDY_HELPER_H_
#define __GREEDY_HELPER_H_

/*类PathFlow定义开始*/
class PathFlow
{
public:
    int src;
    int des;
    int cost;
    int flow;
    int path_length;
    vector<int> edge_path;

    PathFlow();
};

PathFlow::PathFlow()
{
}
/*类PathFlow定义结束*/


/*类Customer定义开始*/
class Customer
{
public:
    int cus_id;
    int demand;
    int supplied;           //已经提供的流量
    bool self_supply;
    vector<PathFlow*> supply_path;   //流量来的路径

    Customer(int cus_id, int demand);
    void print_customer();

};

Customer::Customer(int cus_id, int demand)
{
    this->cus_id = cus_id;
    this->demand = demand;
    this->supplied = 0;
    this->self_supply = false;
}

void Customer::print_customer()
{
    cout<<"customer id: "<<cus_id<<" demand: "<<demand<<" supplied: "<<supplied<<endl;
}

struct customer_cmp
{
    bool operator()(Customer* lhs, Customer* rhs)
    {
        if(lhs->demand - lhs->supplied < rhs->demand - rhs->supplied)
            return true;
        return false;
    }
};
/*类Customer定义结束*/
#endif
