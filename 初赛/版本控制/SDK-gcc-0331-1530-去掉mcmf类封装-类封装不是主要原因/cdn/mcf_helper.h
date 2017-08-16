#ifndef _MCF_HELPER_H_
#define _MCF_HELPER_H_

/*类FlowEdgeC定义开始*/
struct FlowEdgeC
{
public:
    int src;
    int des;
    int next;
    int ori_cap;
    int cap;
    int cost;
    int flow;
    int ori_cost;

    FlowEdgeC() {};
    FlowEdgeC(int src, int des, int next, int cap, int cost): src(src), des(des), next(next), ori_cap(cap), cap(cap), cost(cost), flow(0),ori_cost(cost) {}
    void* operator new(size_t, void* p)    {return p;}
};

#endif
/*类FlowEdgeC定义结束*/
